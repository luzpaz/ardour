/*
 * Copyright (C) 2006-2014 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2007-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2013-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2018 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2016 Julien "_FrnchFrgg_" RIVAUD <frnchfrgg@free.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "pbd/error.h"

#include "ardour/amp.h"
#include "ardour/audioengine.h"
#include "ardour/audiofilesource.h"
#include "ardour/audioplaylist.h"
#include "ardour/audioregion.h"
#include "ardour/debug.h"
#include "ardour/delivery.h"
#include "ardour/disk_reader.h"
#include "ardour/disk_writer.h"
#include "ardour/event_type_map.h"
#include "ardour/io_processor.h"
#include "ardour/meter.h"
#include "ardour/midi_playlist.h"
#include "ardour/midi_region.h"
#include "ardour/monitor_control.h"
#include "ardour/playlist.h"
#include "ardour/playlist_factory.h"
#include "ardour/polarity_processor.h"
#include "ardour/port.h"
#include "ardour/processor.h"
#include "ardour/profile.h"
#include "ardour/region_factory.h"
#include "ardour/record_enable_control.h"
#include "ardour/record_safe_control.h"
#include "ardour/route_group_specialized.h"
#include "ardour/session.h"
#include "ardour/session_playlists.h"
#include "ardour/smf_source.h"
#include "ardour/track.h"
#include "ardour/triggerbox.h"
#include "ardour/types_convert.h"
#include "ardour/utils.h"

#include "pbd/i18n.h"

using namespace std;
using namespace ARDOUR;
using namespace PBD;

Track::Track (Session& sess, string name, PresentationInfo::Flag flag, TrackMode mode, DataType default_type)
	: Route (sess, name, flag, default_type)
	, _record_prepared (false)
	, _mode (mode)
	, _alignment_choice (Automatic)
	, _pending_name_change (false)
{
	_freeze_record.state = NoFreeze;
}

Track::~Track ()
{
	DEBUG_TRACE (DEBUG::Destruction, string_compose ("track %1 destructor\n", _name));

	for (auto const& p : _playlists) {
		if (p) {
			p->clear_time_domain_parent ();
		}
	}

	if (_disk_reader) {
		_disk_reader.reset ();
	}

	if (_disk_writer) {
		_disk_writer.reset ();
	}
}

int
Track::init ()
{
	if (!is_auditioner()) {
		_triggerbox = std::shared_ptr<TriggerBox> (new TriggerBox (_session, data_type ()));
		_triggerbox->set_owner (this);
	}

	if (Route::init ()) {
		return -1;
	}

	DiskIOProcessor::Flag dflags = DiskIOProcessor::Recordable;

	_disk_reader.reset (new DiskReader (_session, *this, name(), Temporal::TimeDomainProvider (Config->get_default_automation_time_domain()), dflags));
	_disk_reader->set_block_size (_session.get_block_size ());
	_disk_reader->set_owner (this);

	_disk_writer.reset (new DiskWriter (_session, *this, name(), dflags));
	_disk_writer->set_block_size (_session.get_block_size ());
	_disk_writer->set_owner (this);

	/* no triggerbox for the auditioner, to avoid visual clutter in
	 * patchbays and elsewhere (or special-case code in those places)
	 */

	set_align_choice_from_io ();

	std::shared_ptr<Route> rp (std::dynamic_pointer_cast<Route> (shared_from_this()));
	std::shared_ptr<Track> rt = std::dynamic_pointer_cast<Track> (rp);

	_record_enable_control.reset (new RecordEnableControl (_session, EventTypeMap::instance().to_symbol (RecEnableAutomation), *this, *this));
	add_control (_record_enable_control);

	_record_safe_control.reset (new RecordSafeControl (_session, EventTypeMap::instance().to_symbol (RecSafeAutomation), *this, *this));
	add_control (_record_safe_control);

	_monitoring_control.reset (new MonitorControl (_session, EventTypeMap::instance().to_symbol (MonitoringAutomation), *this, *this));
	add_control (_monitoring_control);

	if (!name().empty()) {
		/* an empty name means that we are being constructed via
		 * serialized state (XML). Don't create a playlist, because one
		 * will be created or discovered during ::set_state().
		 */
		use_new_playlist (data_type());
		/* set disk-I/O and diskstream name */
		set_name (name ());
	}

	_session.config.ParameterChanged.connect_same_thread (*this, std::bind (&Track::parameter_changed, this, _1));
	_session.RecordStateChanged.connect_same_thread (*this, std::bind (&Track::update_input_meter, this));
	_session.TransportStateChange.connect_same_thread (*this, std::bind (&Track::update_input_meter, this));

	_monitoring_control->Changed.connect_same_thread (*this, std::bind (&Track::monitoring_changed, this, _1, _2));
	_record_safe_control->Changed.connect_same_thread (*this, std::bind (&Track::record_safe_changed, this, _1, _2));
	_record_enable_control->Changed.connect_same_thread (*this, std::bind (&Track::record_enable_changed, this, _1, _2));

	_input->changed.connect_same_thread (*this, std::bind (&Track::input_changed, this));

	_disk_reader->ConfigurationChanged.connect_same_thread (*this, std::bind (&Track::chan_count_changed, this));

	return 0;
}

void
Track::input_changed ()
{
	if (_disk_writer && _alignment_choice == Automatic) {
		set_align_choice_from_io ();
	}
}

void
Track::chan_count_changed ()
{
	ChanCountChanged (); /* EMIT SIGNAL */
}

XMLNode&
Track::playlist_state () const
{
	XMLNode* node = new XMLNode("Route");
	node->set_property("version", CURRENT_SESSION_FILE_VERSION);

	if (_playlists[DataType::AUDIO]) {
		node->set_property (X_("audio-playlist"), _playlists[DataType::AUDIO]->id().to_s());
	}

	if (_playlists[DataType::MIDI]) {
		node->set_property (X_("midi-playlist"), _playlists[DataType::MIDI]->id().to_s());
	}

	return *node;
}

XMLNode&
Track::state (bool save_template) const
{
	XMLNode& root (Route::state (save_template));

	if (_playlists[DataType::AUDIO]) {
		root.set_property (X_("audio-playlist"), _playlists[DataType::AUDIO]->id().to_s());
	}

	if (_playlists[DataType::MIDI]) {
		root.set_property (X_("midi-playlist"), _playlists[DataType::MIDI]->id().to_s());
	}

	root.add_child_nocopy (_monitoring_control->get_state ());
	root.add_child_nocopy (_record_safe_control->get_state ());
	root.add_child_nocopy (_record_enable_control->get_state ());

	if (_saved_meter_point) {
		root.set_property (X_("saved-meter-point"), _saved_meter_point.value ());
	}
	root.set_property (X_("alignment-choice"), _alignment_choice);

	return root;
}

int
Track::set_state (const XMLNode& node, int version)
{
	if (Route::set_state (node, version)) {
		return -1;
	}

	if (version >= 3000 && version < 6000) {
		if (XMLNode* ds_node = find_named_node (node, "Diskstream")) {
			std::string name;
			if (ds_node->get_property ("playlist", name)) {

				ds_node->set_property ("active", true);

				_disk_writer->set_state (*ds_node, version);
				_disk_reader->set_state (*ds_node, version);

				AlignChoice ac;
				if (ds_node->get_property (X_("capture-alignment"), ac)) {
					set_align_choice (ac, true);
				}

				if (std::shared_ptr<AudioPlaylist> pl = std::dynamic_pointer_cast<AudioPlaylist> (_session.playlists()->by_name (name))) {
					use_playlist (DataType::AUDIO, pl);
				}

				if (std::shared_ptr<MidiPlaylist> pl = std::dynamic_pointer_cast<MidiPlaylist> (_session.playlists()->by_name (name))) {
					use_playlist (DataType::MIDI, pl);
				}
			}
		}
	}

	XMLNode* child;
	std::string playlist_id;

	if (node.get_property (X_("audio-playlist"), playlist_id)) {
		find_and_use_playlist (DataType::AUDIO, PBD::ID (playlist_id));
	}

	if (node.get_property (X_("midi-playlist"), playlist_id)) {
		find_and_use_playlist (DataType::MIDI, PBD::ID (playlist_id));
	}

	XMLNodeList nlist = node.children();
	for (XMLNodeConstIterator niter = nlist.begin(); niter != nlist.end(); ++niter) {
		child = *niter;

		if (child->name() == Controllable::xml_node_name) {
			std::string name;
			if (!child->get_property ("name", name)) {
				continue;
			}

			if (name == _record_enable_control->name()) {
				_record_enable_control->set_state (*child, version);
			} else if (name == _record_safe_control->name()) {
				_record_safe_control->set_state (*child, version);
			} else if (name == _monitoring_control->name()) {
				_monitoring_control->set_state (*child, version);
			} else if (name == "recenable" && version <= 3002) {
				float value;
				if (child->get_property ("value", value)) {
					_record_enable_control->set_value (value, Controllable::NoGroup);
				}
			}
		}
	}

	MeterPoint mp;
	if (node.get_property (X_("saved-meter-point"), mp)) {
		_saved_meter_point = mp;
	}

	AlignChoice ac;

	if (node.get_property (X_("alignment-choice"), ac)) {
		set_align_choice (ac, true);
	}

	return 0;
}

Track::FreezeRecord::~FreezeRecord ()
{
	for (vector<FreezeRecordProcessorInfo*>::iterator i = processor_info.begin(); i != processor_info.end(); ++i) {
		delete *i;
	}
}

Track::FreezeState
Track::freeze_state() const
{
	return _freeze_record.state;
}

bool
Track::declick_in_progress () const
{
	return active() && _disk_reader->declick_in_progress ();
}

bool
Track::can_record()
{
	bool will_record = true;
	for (auto const& p : *_input->ports()) {
		if (!p->connected()) {
			will_record = false;
			break;
		}
	}
	return will_record;
}

int
Track::prep_record_enabled (bool yn)
{
	if (yn && _record_safe_control->get_value()) {
		return -1;
	}

	if (!can_be_record_enabled()) {
		return -1;
	}

	bool will_follow;

	if (yn) {
		will_follow = _disk_writer->prep_record_enable ();
	} else {
		will_follow = _disk_writer->prep_record_disable ();
	}

	if (!will_follow) {
		return -1;
	}

	_record_prepared = yn;
	update_input_meter ();

	return 0;
}

void
Track::update_input_meter ()
{
	if (_session.loading ()) {
		return;
	}
	/* meter input if _record_prepared,
	 * except if Rolling, but not recording (master-rec-enable is off) and auto-input is enabled
	 */
	bool monitor_input = false;

	if (_record_prepared) {
		/* actually rolling (no count-in, pre-roll) */
		bool const rolling     = 0 != _session.transport_speed();
		bool const recording   = _session.actively_recording ();
		bool const auto_input  = _session.config.get_auto_input ();

		if (!(rolling && !recording && auto_input)) {
			monitor_input = true;
		}
	}

	if (monitor_input) {
		if (_saved_meter_point) {
			/* already monitoring input */
			return;
		}
		MeterPoint mp = meter_point ();
		if (mp == MeterInput) {
			/* user explicitly monitors input, do nothing */
			return;
		}

		/* keep track of the meter point as it was before we rec-enabled */
		_saved_meter_point = mp;

		if (mp != MeterCustom) {
			set_meter_point (MeterInput);
		}

	} else {
		if (!_saved_meter_point) {
			return;
		}
		if (_saved_meter_point != MeterCustom) {
			set_meter_point (_saved_meter_point.value ());
		}
		_saved_meter_point.reset ();
	}
}

void
Track::record_enable_changed (bool, Controllable::GroupControlDisposition)
{
	bool yn = _record_enable_control->get_value();

	_disk_writer->set_record_enabled (yn);
	_triggerbox->set_record_enabled (yn);
}

void
Track::record_safe_changed (bool, Controllable::GroupControlDisposition)
{
	_disk_writer->set_record_safe (_record_safe_control->get_value());
}

bool
Track::can_be_record_safe ()
{
	return !_record_enable_control->get_value() && _disk_writer && _session.writable() && (_freeze_record.state != Frozen);
}

bool
Track::can_be_record_enabled ()
{
	return !_record_safe_control->get_value() && _disk_writer && !_disk_writer->record_safe() && _session.writable() && (_freeze_record.state != Frozen) && (!_triggerbox || !_triggerbox->record_enabled());
}

void
Track::parameter_changed (string const & p)
{
	if (p == "track-name-number") {
		resync_take_name ();
	}
	else if (p == "track-name-take") {
		resync_take_name ();
	}
	else if (p == "take-name") {
		if (_session.config.get_track_name_take()) {
			resync_take_name ();
		}
	} else if (p == "auto-input") {
		update_input_meter ();
	}
}

int
Track::resync_take_name (std::string n)
{
	if (n.empty ()) {
		n = name ();
	}

	if (_record_enable_control->get_value() && _session.actively_recording ()) {
		_pending_name_change = true;
		return -1;
	}

	string diskstream_name = "";
	if (_session.config.get_track_name_take () && !_session.config.get_take_name ().empty()) {
		// Note: any text is fine, legalize_for_path() fixes this later
		diskstream_name += _session.config.get_take_name ();
		diskstream_name += "_";
	}
	const int64_t tracknumber = track_number();
	if (tracknumber > 0 && _session.config.get_track_name_number()) {
		char num[64], fmt[10];
		snprintf(fmt, sizeof(fmt), "%%0%d" PRId64, _session.track_number_decimals());
		snprintf(num, sizeof(num), fmt, tracknumber);
		diskstream_name += num;
		diskstream_name += "_";
	}

	diskstream_name += n;

	if (diskstream_name == _diskstream_name) {
		return 1;
	}

	_diskstream_name = diskstream_name;
	_disk_writer->set_write_source_name (diskstream_name);
	return 0;
}

bool
Track::set_name (const string& str)
{
	if (_record_enable_control->get_value()) {
		/* cannot rename rec-armed track - see also Track::resync_take_name */
		return false;
	}

	if (str.empty ()) {
		return false;
	}

	string newname = Route::ensure_track_or_route_name (str);

	if (newname == name()) {
		return true;
	}

	switch (resync_take_name (newname)) {
		case -1:
			return false;
		case 1:
			return true;
		default:
			break;
	}

	std::shared_ptr<Track> me = std::dynamic_pointer_cast<Track> (shared_from_this ());

	_disk_reader->set_name (newname);
	_disk_writer->set_name (newname);


	/* When creating a track during session-load, do not change playlist's name.
	 *
	 * Changing the playlist name from 'toBeResetFroXML' breaks loading
	 * Ardour v2..5 sessions. Older versions of Arodur identified playlist
	 * by name, and this causes duplicate names and name conflicts.
	 * (new track name -> new playlist name  != old playlist)
	 */
	if (_session.loading ()) {
		return Route::set_name (newname);
	}

	for (uint32_t n = 0; n < DataType::num_types; ++n) {
		if (!_playlists[n]) {
			continue;
		}
		if (_playlists[n]->all_regions_empty () && _session.playlists()->playlists_for_track (me).size() == 1) {
			/* Only rename the the playlist if
			 * a) the playlist has never had a region added to it and
			 * b) there is only one playlist for this track.
			 *
			 * If (a) is not followed, people can get confused if, say,
			 * they have notes about a playlist with a given name and then
			 * it changes (see mantis #4759).
			 *
			 * If (b) is not followed, we rename the current playlist and not
			 * the other ones, which is a bit confusing (see mantis #4977).
			 */
			_playlists[n]->set_name (newname);
		}
	}

	return Route::set_name (newname);
}

std::shared_ptr<Playlist>
Track::playlist ()
{
	return _playlists[data_type()];
}

void
Track::request_input_monitoring (bool m)
{
	for (auto const& p : *_input->ports()) {
		AudioEngine::instance()->request_input_monitoring (p->name(), m);
	}
}

void
Track::ensure_input_monitoring (bool m)
{
	for (auto const& p : *_input->ports()) {
		AudioEngine::instance()->ensure_input_monitoring (p->name(), m);
	}
}

list<std::shared_ptr<Source> > &
Track::last_capture_sources ()
{
	return _disk_writer->last_capture_sources ();
}

void
Track::reset_last_capture_sources ()
{
	_disk_writer->reset_last_capture_sources ();
}

std::string
Track::steal_write_source_name()
{
        return _disk_writer->steal_write_source_name ();
}

void
Track::reset_write_sources (bool mark_write_complete)
{
	_disk_writer->reset_write_sources (mark_write_complete);
}

float
Track::playback_buffer_load () const
{
	return _disk_reader->buffer_load ();
}

float
Track::capture_buffer_load () const
{
	return _disk_writer->buffer_load ();
}

int
Track::do_refill ()
{
	return _disk_reader->do_refill ();
}

int
Track::do_flush (RunContext c, bool force)
{
	return _disk_writer->do_flush (c, force);
}

void
Track::set_pending_overwrite (OverwriteReason why)
{
	_disk_reader->set_pending_overwrite (why);
}

int
Track::seek (samplepos_t p, bool complete_refill)
{
	if (_disk_reader->seek (p, complete_refill)) {
		return -1;
	}
	return _disk_writer->seek (p, complete_refill);
}

bool
Track::can_internal_playback_seek (samplecnt_t p)
{
	return _disk_reader->can_internal_playback_seek (p);
}

void
Track::internal_playback_seek (samplecnt_t p)
{
	return _disk_reader->internal_playback_seek (p);
}

void
Track::non_realtime_locate (samplepos_t p)
{
	Route::non_realtime_locate (p);
}

bool
Track::overwrite_existing_buffers ()
{
	return _disk_reader->overwrite_existing_buffers ();
}

samplecnt_t
Track::get_captured_samples (uint32_t n) const
{
	return _disk_writer->get_captured_samples (n);
}

void
Track::transport_looped (samplepos_t p)
{
	return _disk_writer->transport_looped (p);
}

void
Track::transport_stopped_wallclock (struct tm & n, time_t t, bool g)
{
	_disk_writer->transport_stopped_wallclock (n, t, g);

	if (_pending_name_change) {
		resync_take_name ();
		_pending_name_change = false;
	}
}

void
Track::mark_capture_xrun ()
{
	if (_disk_writer->record_enabled ()) {
		_disk_writer->mark_capture_xrun ();
	}
}

bool
Track::pending_overwrite () const
{
	return _disk_reader->pending_overwrite ();
}

void
Track::set_slaved (bool s)
{
	_disk_reader->set_slaved (s);
	_disk_writer->set_slaved (s);
}

ChanCount
Track::n_channels ()
{
	return _disk_reader->output_streams();
}

samplepos_t
Track::get_capture_start_sample (uint32_t n) const
{
	return _disk_writer->get_capture_start_sample (n);
}

AlignStyle
Track::alignment_style () const
{
	return _disk_writer->alignment_style ();
}

AlignChoice
Track::alignment_choice () const
{
	return _alignment_choice;
}

samplepos_t
Track::current_capture_start () const
{
	return _disk_writer->current_capture_start ();
}

samplepos_t
Track::current_capture_end () const
{
	return _disk_writer->current_capture_end ();
}

void
Track::playlist_modified ()
{
	_disk_reader->playlist_modified ();
}

int
Track::find_and_use_playlist (DataType dt, PBD::ID const & id)
{
	std::shared_ptr<Playlist> playlist;

	if ((playlist = _session.playlists()->by_id (id)) == 0) {
		return -1;
	}

	if (!playlist) {
		error << string_compose(_("DiskIOProcessor: \"%1\" isn't an playlist"), id.to_s()) << endmsg;
		return -1;
	}

	return use_playlist (dt, playlist);
}

int
Track::use_playlist (DataType dt, std::shared_ptr<Playlist> p, bool set_orig)
{
	int ret;

	if ((ret = _disk_reader->use_playlist (dt, p)) == 0) {
		if ((ret = _disk_writer->use_playlist (dt, p)) == 0) {
			if (set_orig) {
				p->set_orig_track_id (id());
			}
		}
	}

	std::shared_ptr<Playlist> old = _playlists[dt];

	if (ret == 0) {
		_playlists[dt] = p;
	}

	if (old) {
		std::shared_ptr<RegionList> rl (new RegionList (old->region_list_property ().rlist ()));
		if (rl->size () > 0) {
			Region::RegionsPropertyChanged (rl, Properties::hidden);
		}
		/* we don't know for certain that we controlled the old
		 * playlist's time domain, but it's a pretty good guess. If it
		 * has an actual parent, revert to using its parent's domain
		 */
		if (old->time_domain_parent()) {
			old->clear_time_domain_parent ();
		}
	}

	if (p) {
		std::shared_ptr<RegionList> rl (new RegionList (p->region_list_property ().rlist ()));
		if (rl->size () > 0) {
			Region::RegionsPropertyChanged (rl, Properties::hidden);
		}

		/* If the playlist has no time domain parent or its parent is
		 * the session, reset to the explicit time domain of this
		 * track.
		 */

		if (!p->time_domain_parent() || p->time_domain_parent() == &_session) {
			/* XXX DANGER : track could go away leaving playlist
			 * with dead parent time domain provider
			 */
			p->set_time_domain_parent (*this);
		}
	}

	_session.set_dirty ();
	PlaylistChanged (); /* EMIT SIGNAL */

	return ret;
}

int
Track::use_copy_playlist ()
{
	assert (_playlists[data_type()]);

	if (_playlists[data_type()] == 0) {
		error << string_compose(_("DiskIOProcessor %1: there is no existing playlist to make a copy of!"), _name) << endmsg;
		return -1;
	}

	string newname;
	std::shared_ptr<Playlist> playlist;

	newname = Playlist::bump_name (_playlists[data_type()]->name(), _session);

	if ((playlist = PlaylistFactory::create (_playlists[data_type()], newname)) == 0) {
		return -1;
	}

	playlist->reset_shares();

	int rv = use_playlist (data_type(), playlist);
	PlaylistAdded (); /* EMIT SIGNAL */
	return rv;
}

int
Track::use_new_playlist (DataType dt)
{
	string newname;
	std::shared_ptr<Playlist> playlist = _playlists[dt];

	if (playlist) {
		newname = Playlist::bump_name (playlist->name(), _session);
	} else {
		newname = Playlist::bump_name (_name, _session);
	}

	playlist = PlaylistFactory::create (dt, _session, newname, is_private_route());

	if (!playlist) {
		return -1;
	}

	int rv = use_playlist (dt, playlist);
	PlaylistAdded (); /* EMIT SIGNAL */
	return rv;
}

void
Track::set_align_choice (AlignChoice ac, bool force)
{
	_alignment_choice = ac;
	switch (ac) {
		case Automatic:
			set_align_choice_from_io ();
			break;
		case UseCaptureTime:
			_disk_writer->set_align_style (CaptureTime, force);
			break;
		case UseExistingMaterial:
			_disk_writer->set_align_style (ExistingMaterial, force);
			break;
	}
}

void
Track::set_align_style (AlignStyle s, bool force)
{
	_disk_writer->set_align_style (s, force);
}

void
Track::set_align_choice_from_io ()
{
	bool have_physical = false;

	if (_input) {
		uint32_t n = 0;
		std::shared_ptr<Port> p;

		while (0 != (p = _input->nth (n++))) {
			/* In case of JACK all ports not owned by Ardour may be re-sampled,
			 * and latency is added. External JACK ports need to be treated
			 * like physical ports: I/O latency needs to be taken into account.
			 *
			 * When not using JACK, all external ports are physical ports
			 * so this is a NO-OP for other backends.
			 */
			if (p->externally_connected () || p->physically_connected ()) {
				have_physical = true;
				break;
			}
		}
	}

#ifdef MIXBUS
	// compensate for latency when bouncing from master or mixbus.
	// we need to use "ExistingMaterial" to pick up the master bus' latency
	// see also Route::direct_feeds_according_to_reality
	IOVector ios;
	ios.push_back (_input);
	if (_session.master_out() && ios.fed_by (_session.master_out()->output())) {
		have_physical = true;
	}
	for (uint32_t n = 0; n < NUM_MIXBUSES && !have_physical; ++n) {
		if (_session.get_mixbus (n) && ios.fed_by (_session.get_mixbus(n)->output())) {
			have_physical = true;
		}
	}
#endif


	if (have_physical) {
		_disk_writer->set_align_style (ExistingMaterial);
	} else {
		_disk_writer->set_align_style (CaptureTime);
	}
}

void
Track::set_block_size (pframes_t n)
{
	Route::set_block_size (n);
	_disk_reader->set_block_size (n);
	_disk_writer->set_block_size (n);
}

void
Track::adjust_playback_buffering ()
{
        if (_disk_reader) {
                _disk_reader->adjust_buffering ();
        }
}

void
Track::adjust_capture_buffering ()
{
        if (_disk_writer) {
                _disk_writer->adjust_buffering ();
        }
}

void
Track::monitoring_changed (bool, Controllable::GroupControlDisposition)
{
	for (ProcessorList::iterator i = _processors.begin(); i != _processors.end(); ++i) {
		(*i)->monitoring_changed ();
	}
}

bool
Track::set_processor_state (XMLNode const& node, int version, XMLProperty const* prop, ProcessorList& new_order, bool& must_configure)
{
	if (Route::set_processor_state (node, version, prop, new_order, must_configure)) {
		return true;
	}

	if (prop->value() == "diskreader") {
		if (_disk_reader) {
			_disk_reader->set_state (node, version);
			new_order.push_back (_disk_reader);
			return true;
		}
	} else if (prop->value() == "diskwriter") {
		if (_disk_writer) {
			_disk_writer->set_state (node, version);
			new_order.push_back (_disk_writer);
			return true;
		}
	}

	error << string_compose(_("unknown Processor type \"%1\"; ignored"), prop->value()) << endmsg;
	return false;
}

std::shared_ptr<Region>
Track::bounce (InterThreadInfo& itt, std::string const& name)
{
	return bounce_range (_session.current_start_sample(), _session.current_end_sample(), itt, main_outs(), false, name);
}

std::shared_ptr<Region>
Track::bounce_range (samplepos_t start,
                          samplepos_t end,
                          InterThreadInfo& itt,
                          std::shared_ptr<Processor> endpoint,
                          bool include_endpoint,
                          std::string const& nm, bool prefix_track_name)
{
	std::string source_name;

	if (prefix_track_name && nm.length() > 0) {
		source_name = string_compose ("%1 - %2", name(), nm);
	} else {
		source_name = nm;
	}

	vector<std::shared_ptr<Source> > srcs;
	return _session.write_one_track (*this, start, end, false, srcs, itt, endpoint, include_endpoint, false, false, source_name, nm);
}

void
Track::use_captured_sources (SourceList& srcs, CaptureInfos const & capture_info)
{
	if (srcs.empty()) {
		return;
	}

	std::shared_ptr<AudioFileSource> afs = std::dynamic_pointer_cast<AudioFileSource> (srcs.front());
	std::shared_ptr<SMFSource> mfs = std::dynamic_pointer_cast<SMFSource> (srcs.front());

	if (afs) {
		use_captured_audio_sources (srcs, capture_info);
	}

	if (mfs) {
		use_captured_midi_sources (srcs, capture_info);
	}
}

void
Track::use_captured_midi_sources (SourceList& srcs, CaptureInfos const & capture_info)
{
	if (srcs.empty() || data_type() != DataType::MIDI) {
		return;
	}

	/* There is an assumption here that we have only a single MIDI file */

	std::shared_ptr<SMFSource> mfs = std::dynamic_pointer_cast<SMFSource> (srcs.front());
	std::shared_ptr<Playlist> pl = _playlists[DataType::MIDI];
	std::shared_ptr<MidiRegion> midi_region;
	CaptureInfos::const_iterator ci;

	if (!mfs || !pl) {
		return;
	}

	RecordMode rmode = _session.config.get_record_mode ();

	samplecnt_t total_capture = 0;

	for (total_capture = 0, ci = capture_info.begin(); ci != capture_info.end(); ++ci) {
		total_capture += (*ci)->samples;
	}

	/* we will want to be able to keep (over)writing the source
	   but we don't want it to be removable. this also differs
	   from the audio situation, where the source at this point
	   must be considered immutable. luckily, we can rely on
	   MidiSource::mark_streaming_write_completed() to have
	   already done the necessary work for that.
	*/

	string whole_file_region_name;
	whole_file_region_name = region_name_from_path (mfs->name(), true);

	/* Register a new region with the Session that
	   describes the entire source. Do this first
	   so that any sub-regions will obviously be
	   children of this one (later!)
	*/

	try {
		PropertyList plist;

		plist.add (Properties::name, whole_file_region_name);
		plist.add (Properties::whole_file, true);
		plist.add (Properties::automatic, true);
		plist.add (Properties::opaque, rmode != RecSoundOnSound);
		plist.add (Properties::start, timecnt_t (Temporal::BeatTime));
		plist.add (Properties::length, mfs->length());
		plist.add (Properties::layer, 0);

		std::shared_ptr<Region> rx (RegionFactory::create (srcs, plist));

		midi_region = std::dynamic_pointer_cast<MidiRegion> (rx);
		midi_region->special_set_position (timepos_t (capture_info.front()->start));
	}

	catch (failed_constructor& err) {
		error << string_compose(_("%1: could not create region for complete midi file"), _name) << endmsg;
		/* XXX what now? */
	}

	pl->clear_changes ();
	pl->freeze ();

	/* Session sample time of the initial capture in this pass, which is where the source starts */
	samplepos_t initial_capture = 0;
	if (!capture_info.empty()) {
		initial_capture = capture_info.front()->start;
	}

	const samplepos_t preroll_off = _session.preroll_record_trim_len ();
	const timepos_t cstart (timepos_t (capture_info.front()->start).beats());

	int cnt = 0;
	for (ci = capture_info.begin(); ci != capture_info.end(); ++ci, ++cnt) {

		string region_name;

		RegionFactory::region_name (region_name, mfs->name(), false);

		DEBUG_TRACE (DEBUG::CaptureAlignment, string_compose ("%1 capture start @ %2 length %3 add new region %4\n",
		                                                      _name, (*ci)->start, (*ci)->samples, region_name));


		// cerr << _name << ": based on ci of " << (*ci)->start << " for " << (*ci)->samples << " start: " << (*ci)->loop_offset << " add MIDI region\n";

		try {
			PropertyList plist;

			/* start of this region is the offset between the start of its capture and the start of the whole pass */
			samplecnt_t start_off = (*ci)->start - initial_capture + (*ci)->loop_offset;
			timepos_t s;
			timecnt_t l;

			if (time_domain() == Temporal::BeatTime) {

				const timepos_t ss (start_off);
				/* 2nd argument is the timeline position of the
				 * start of the region in samples. We have to
				 * get this right so that the conversion of
				 * the capture duration (samples) to beats is
				 * using the actual position where the region
				 * will end up, rather than using its
				 * source-relative start offset as a timeline position.
				 *
				 * This matters if the region ought to cover
				 * part of the timeline where the tempo is
				 * different from the value at the natural
				 * position of the source.
				 */
				const timecnt_t ll ((*ci)->samples, timepos_t (initial_capture + start_off));

				s = timepos_t (ss.beats());
				l = timecnt_t (ll.beats(), s);

			} else {

				s = timepos_t (start_off);
				l = timecnt_t ((*ci)->samples, s);
			}

			plist.add (Properties::start, s);
			plist.add (Properties::length, l);
			plist.add (Properties::opaque, rmode != RecSoundOnSound);
			plist.add (Properties::name, region_name);
			plist.add (Properties::reg_group, Region::get_retained_group_id (cnt));

			std::shared_ptr<Region> rx (RegionFactory::create (srcs, plist));
			midi_region = std::dynamic_pointer_cast<MidiRegion> (rx);
			if (preroll_off > 0) {
				midi_region->trim_front (timepos_t ((*ci)->start - initial_capture + preroll_off));
			}
		}

		catch (failed_constructor& err) {
			error << string_compose (_("%1: could not create region for captured data!"), name()) << endmsg;
			continue; /* XXX is this OK? */
		}

		if (time_domain() == Temporal::BeatTime) {
			const timepos_t b ((*ci)->start + preroll_off);
			pl->add_region (midi_region, timepos_t (b.beats()), 1, rmode == RecNonLayered);
		} else {
			pl->add_region (midi_region, timepos_t ((*ci)->start + preroll_off), 1, rmode == RecNonLayered);
		}
	}

	pl->thaw ();
	_session.add_command (new StatefulDiffCommand (pl));
}

void
Track::use_captured_audio_sources (SourceList& srcs, CaptureInfos const & capture_info)
{
	if (srcs.empty() || data_type() != DataType::AUDIO) {
		return;
	}

	std::shared_ptr<AudioFileSource> afs = std::dynamic_pointer_cast<AudioFileSource> (srcs.front());
	std::shared_ptr<Playlist> pl = _playlists[DataType::AUDIO];
	std::shared_ptr<AudioRegion> region;

	if (!afs || !pl) {
		return;
	}

	string whole_file_region_name;
	whole_file_region_name = region_name_from_path (afs->name(), true);

	/* Register a new region with the Session that
	   describes the entire source. Do this first
	   so that any sub-regions will obviously be
	   children of this one (later!)
	*/

	RecordMode rmode = _session.config.get_record_mode ();

	try {
		PropertyList plist;

		plist.add (Properties::start, timecnt_t (afs->last_capture_start_sample(), timepos_t (Temporal::AudioTime)));
		plist.add (Properties::length, afs->length());
		plist.add (Properties::name, whole_file_region_name);
		plist.add (Properties::opaque, rmode != RecSoundOnSound);
		std::shared_ptr<Region> rx (RegionFactory::create (srcs, plist));
		rx->set_automatic (true);
		rx->set_whole_file (true);

		region = std::dynamic_pointer_cast<AudioRegion> (rx);
		region->special_set_position (timepos_t (afs->natural_position()));
	}


	catch (failed_constructor& err) {
		error << string_compose(_("%1: could not create region for complete audio file"), _name) << endmsg;
		/* XXX what now? */
	}

	/* If this playlist doesn't already have a pgroup (a new track won't) then
	 * assign it one, using the take-id of the first recording)
	 */
	if (pl->pgroup_id().length() == 0) {
		pl->set_pgroup_id (afs->take_id ());
	}

	pl->clear_changes ();
	pl->set_capture_insertion_in_progress (true);
	pl->freeze ();

	const samplepos_t preroll_off = _session.preroll_record_trim_len ();
	samplecnt_t buffer_position = afs->last_capture_start_sample ();
	CaptureInfos::const_iterator ci;

	int cnt = 0;
	for (ci = capture_info.begin(); ci != capture_info.end(); ++ci, ++cnt) {

		string region_name;

		RegionFactory::region_name (region_name, whole_file_region_name, false);

		DEBUG_TRACE (DEBUG::CaptureAlignment, string_compose ("%1 capture bufpos %5 start @ %2 length %3 add new region %4\n",
		                                                      _name, (*ci)->start, (*ci)->samples, region_name, buffer_position));

		try {

			PropertyList plist;

			plist.add (Properties::start, timecnt_t (buffer_position, timepos_t::zero (false)));
			plist.add (Properties::length, timecnt_t ((*ci)->samples, timepos_t::zero (false)));
			plist.add (Properties::name, region_name);
			plist.add (Properties::opaque, rmode != RecSoundOnSound);
			plist.add (Properties::reg_group, Region::get_retained_group_id (cnt));

			std::shared_ptr<Region> rx (RegionFactory::create (srcs, plist));
			region = std::dynamic_pointer_cast<AudioRegion> (rx);
			if (preroll_off > 0) {
				region->trim_front (timepos_t (buffer_position + preroll_off));
			}
		}

		catch (failed_constructor& err) {
			error << _("AudioDiskstream: could not create region for captured audio!") << endmsg;
			continue; /* XXX is this OK? */
		}

		pl->add_region (region, timepos_t ((*ci)->start + preroll_off), 1, RecNonLayered == rmode);
		pl->set_layer (region, DBL_MAX);

		buffer_position += (*ci)->samples;
	}

	pl->thaw ();
	pl->set_capture_insertion_in_progress (false);
	_session.add_command (new StatefulDiffCommand (pl));
}

void
Track::time_domain_changed ()
{
	Route::time_domain_changed ();

	std::shared_ptr<Playlist> pl = _playlists[DataType::AUDIO];
	if (pl) {
		if (pl->time_domain_parent() == this) {
			pl->time_domain_changed ();
		}
	}
	pl = _playlists[DataType::MIDI];
	if (pl) {
		if (pl->time_domain_parent() == this) {
			pl->time_domain_changed ();
		}
	}
}
