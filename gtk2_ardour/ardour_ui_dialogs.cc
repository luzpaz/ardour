/*
 * Copyright (C) 2005-2006 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2005-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2006-2016 David Robillard <d@drobilla.net>
 * Copyright (C) 2006 Hans Fugal <hans@fugal.net>
 * Copyright (C) 2006 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2007-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2013-2020 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014-2018 Ben Loftis <ben@harrisonconsoles.com>
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

/* This file contains any ARDOUR_UI methods that require knowledge of
   the various dialog boxes, and exists so that no compilation dependency
   exists between the main ARDOUR_UI modules and their respective classes.
   This is to cut down on the compile times.  It also helps with my sanity.
*/

#include <vector>

#include <ytkmm/treemodelfilter.h>

#include "pbd/convert.h"

#include "ardour/audioengine.h"
#include "ardour/automation_watch.h"
#include "ardour/control_protocol_manager.h"
#include "ardour/profile.h"
#include "ardour/session.h"

#include "control_protocol/control_protocol.h"
#include "control_protocol/basic_ui.h"

#include "gtkmm2ext/keyboard.h"
#include "gtkmm2ext/utils.h"

#include "actions.h"
#include "add_route_dialog.h"
#include "add_video_dialog.h"
#include "ardour_ui.h"
#include "big_clock_window.h"
#include "big_transport_window.h"
#include "bundle_manager.h"
#include "dsp_stats_window.h"
#include "global_port_matrix.h"
#include "gui_object.h"
#include "gui_thread.h"
#include "keyeditor.h"
#include "library_download_dialog.h"
#include "location_ui.h"
#include "lua_script_manager.h"
#include "luawindow.h"
#include "main_clock.h"
#include "meterbridge.h"
#include "meter_patterns.h"
#include "monitor_section.h"
#include "midi_tracer.h"
#include "mixer_ui.h"
#include "pianoroll_window.h"
#include "plugin_dspload_window.h"
#include "plugin_manager_ui.h"
#include "public_editor.h"
#include "processor_box.h"
#include "rc_option_editor.h"
#include "recorder_ui.h"
#include "route_params_ui.h"
#include "rta_window.h"
#include "shuttle_control.h"
#include "session_option_editor.h"
#include "speaker_dialog.h"
#include "splash.h"
#include "sfdb_ui.h"
#include "timers.h"
#include "transport_masters_dialog.h"
#include "trigger_page.h"
#include "virtual_keyboard_window.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace PBD;
using namespace Glib;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace ArdourWidgets;

void
ARDOUR_UI::set_session (Session *s)
{
	SessionHandlePtr::set_session (s);

	if (_basic_ui) {
		delete _basic_ui;
	}

	_basic_ui = new BasicUI (*s);

	/* adjust sensitivity of menu bar options to reflect presence/absence
	 * of session
	 */

	ActionManager::set_sensitive (ActionManager::session_sensitive_actions, _session);
	ActionManager::set_sensitive (ActionManager::write_sensitive_actions, _session ? _session->writable() : false);

	if (_session && _session->locations()->num_range_markers()) {
		ActionManager::set_sensitive (ActionManager::range_sensitive_actions, true);
	} else {
		ActionManager::set_sensitive (ActionManager::range_sensitive_actions, false);
	}

	update_path_label ();
	update_sample_rate ();

	if (!_session) {
		/* Session option editor cannot exist across change-of-session */
		session_option_editor.drop_window ();
		/* Ditto for AddVideoDialog */
		add_video_dialog.drop_window ();
		/* screensaver + layered button sensitivity */
		map_transport_state ();
		return;
	}

	const XMLNode* node = _session->extra_xml (X_("UI"));

	if (node) {
		const XMLNodeList& children = node->children();
		for (XMLNodeList::const_iterator i = children.begin(); i != children.end(); ++i) {
			if ((*i)->name() == GUIObjectState::xml_node_name) {
				gui_object_state->load (**i);
				break;
			}
		}
	}

	WM::Manager::instance().set_session (s);
	apply_window_settings (false);

	AutomationWatch::instance().set_session (s);

	primary_clock->set_session (s);
	secondary_clock->set_session (s);
	big_clock->set_session (s);
	video_timeline->set_session (s);
	rc_option_editor->set_session (s);

	roll_controllable->set_session (s);
	stop_controllable->set_session (s);
	goto_start_controllable->set_session (s);
	goto_end_controllable->set_session (s);
	auto_loop_controllable->set_session (s);
	play_selection_controllable->set_session (s);
	rec_controllable->set_session (s);

	/* allow wastebasket flush again */

	Glib::RefPtr<Action> act = ActionManager::get_action (X_("Main"), X_("FlushWastebasket"));
	if (act) {
		act->set_sensitive (true);
	}

	/* there are never any selections on startup */

	ActionManager::set_sensitive (ActionManager::time_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::track_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::route_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::bus_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::vca_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::stripable_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::line_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::point_selection_sensitive_actions, false);
	ActionManager::set_sensitive (ActionManager::playlist_selection_sensitive_actions, false);

	setup_session_options ();

	blink_connection = Timers::blink_connect (sigc::mem_fun(*this, &ARDOUR_UI::blink_handler));

	_session->SaveSessionRequested.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::save_session_at_its_request, this, _1), gui_context());
	_session->StateSaved.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::update_title, this), gui_context());
	_session->StateSaved.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::update_path_label, this), gui_context());
	_session->RecordStateChanged.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::record_state_changed, this), gui_context());
	_session->TransportStateChange.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::map_transport_state, this), gui_context());
	_session->DirtyChanged.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::session_dirty_changed, this), gui_context());
	_session->LatencyUpdated.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::session_latency_updated, this, _1), gui_context());

	_session->PunchLoopConstraintChange.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::set_punch_sensitivity, this), gui_context());
	_session->auto_punch_location_changed.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::set_punch_sensitivity, this), gui_context ());

	_session->Xrun.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::xrun_handler, this, _1), gui_context());
	_session->locations()->added.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::handle_locations_change, this, _1), gui_context());
	_session->locations()->removed.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::handle_locations_change, this, _1), gui_context());
	_session->config.ParameterChanged.connect (_session_connections, MISSING_INVALIDATOR, std::bind (&ARDOUR_UI::session_parameter_changed, this, _1), gui_context ());

	/* Clocks are on by default after we are connected to a session, so show that here.
	*/

	connect_dependents_to_session (s);

	/* listen to clock mode changes. don't do this earlier because otherwise as the clocks
	   restore their modes or are explicitly set, we will cause the "new" mode to be saved
	   back to the session XML ("Extra") state.
	 */

	Glib::signal_idle().connect (sigc::mem_fun (*this, &ARDOUR_UI::first_idle));

	start_clocking ();

	map_transport_state ();
	set_punch_sensitivity ();

	second_connection = Timers::second_connect (sigc::mem_fun(*this, &ARDOUR_UI::every_second));
	point_one_second_connection = Timers::rapid_connect (sigc::mem_fun(*this, &ARDOUR_UI::every_point_one_seconds));
	set_fps_timeout_connection();

	update_format ();

	update_title ();
}

int
ARDOUR_UI::unload_session (bool hide_stuff, bool force_unload)
{
	if (_session) {
		ARDOUR_UI::instance()->video_timeline->sync_session_state();

		/* Unconditionally save session-specific GUI settings:
		 * Playhead position, zoom/scroll with stationary PH,
		 * window and pane positions, etc.
		 *
		 * While many GUI operations immediately cause an instant.xml
		 * save, changing the playhead-pos in particular does not,
		 * nor mark the session dirty.
		 */
		save_ardour_state ();
	}

	if (!force_unload && _session && _session->dirty()) {
		std::vector<std::string> actions;
		actions.push_back (_("Don't close"));
		if (_session->unnamed()) {
			actions.push_back (_("Discard"));
		} else {
			actions.push_back (_("Just close"));
		}
		actions.push_back (_("Save and close"));

		switch (ask_about_saving_session (actions)) {
		case -1:
			// cancel
			return 1;
		case 1:
			// save and continue (and handle unnamed sessions)
			if (_session->unnamed()) {
				rename_session (true);
			}
			_session->save_state ("");
			break;
		case 0:
			// discard/don't save
			if (_session->unnamed()) {
				ask_about_scratch_deletion ();
			}
			break;
		}
	} else if (!force_unload && _session && _session->unnamed()) {
		ask_about_scratch_deletion ();
	}


	{
		// tear down session specific CPI (owned by rc_config_editor which can remain)
		ControlProtocolManager& m = ControlProtocolManager::instance ();
		for (std::list<ControlProtocolInfo*>::iterator i = m.control_protocol_info.begin(); i != m.control_protocol_info.end(); ++i) {
			if (*i && (*i)->protocol && (*i)->protocol->has_editor ()) {
				(*i)->protocol->tear_down_gui ();
			}
		}
	}

	if (hide_stuff) {
		close_all_dialogs ();
		editor->hide ();
		mixer->hide ();
		meterbridge->hide ();
		audio_port_matrix->hide();
		midi_port_matrix->hide();
		route_params->hide();
	}

	second_connection.disconnect ();
	point_one_second_connection.disconnect ();
	fps_connection.disconnect();

	ActionManager::set_sensitive (ActionManager::session_sensitive_actions, false);

	if (ARDOUR_UI::instance()->video_timeline) {
		ARDOUR_UI::instance()->video_timeline->close_session();
	}

	stop_clocking ();

	/* drop everything attached to the blink signal */

	blink_connection.disconnect ();

	ARDOUR::Session* session_to_delete = _session;
	_session = 0;
	delete session_to_delete;

	update_title ();

	return 0;
}

void
ARDOUR_UI::toggle_editor_and_mixer ()
{
	if (editor->tabbed() && mixer->tabbed()) {
		/* both in the same window */
		if (_tabs.get_current_page() == _tabs.page_num (editor->contents())) {
			_tabs.set_current_page (_tabs.page_num (mixer->contents()));
		} else if (_tabs.get_current_page() == _tabs.page_num (mixer->contents())) {
			_tabs.set_current_page (_tabs.page_num (editor->contents()));
		} else {
			/* go to mixer */
			_tabs.set_current_page (_tabs.page_num (mixer->contents()));
		}
		return;
	}


	if (editor->tabbed() && !mixer->tabbed()) {
		/* editor is tabbed, mixer is not */

		Gtk::Window* mwin = mixer->current_toplevel ();

		if (!mwin) {
			/* mixer's own window doesn't exist */
			mixer->make_visible ();
		} else if (!mwin->get_mapped ()) {
			/* mixer's own window exists but isn't mapped */
			mixer->make_visible ();
		} else {
			/* mixer window is mapped, editor is visible as tab */
			Gtk::Widget* f = mwin->get_focus();
			if (f && f->has_focus()) {
				/* mixer has focus, switch to editor */
				editor->make_visible ();
			} else {
				mixer->make_visible ();
			}
		}
		return;
	}

	if (!editor->tabbed() && mixer->tabbed()) {
		/* mixer is tabbed, editor is not */

		Gtk::Window* ewin = editor->current_toplevel ();

		if (!ewin) {
			/* mixer's own window doesn't exist */
			editor->make_visible ();
		} else if (!ewin->get_mapped ()) {
			/* editor's own window exists but isn't mapped */
			editor->make_visible ();
		} else {
			/* editor window is mapped, mixer is visible as tab */
			Gtk::Widget* f = ewin->get_focus();
			if (f && f->has_focus()) {
				/* editor has focus, switch to mixer */
				mixer->make_visible ();
			} else {
				editor->make_visible ();
			}
		}
		return;
	}
}

void
ARDOUR_UI::step_up_through_tabs ()
{
	std::vector<Tabbable*> candidates;

	/* this list must match the order of visibility buttons */

	if (recorder->tabbed()) {
		candidates.push_back (recorder);
	}

	if (editor->tabbed()) {
		candidates.push_back (editor);
	}

	if (mixer->tabbed()) {
		candidates.push_back (mixer);
	}

	if (trigger_page->tabbed()) {
		candidates.push_back (trigger_page);
	}

	if (rc_option_editor->tabbed()) {
		candidates.push_back (rc_option_editor);
	}

	if (candidates.size() < 2) {
		/* nothing to be done with zero or one visible in tabs */
		return;
	}

	std::vector<Tabbable*>::iterator prev = candidates.end();
	std::vector<Tabbable*>::iterator i;
	Gtk::Widget* w = _tabs.get_nth_page (_tabs.get_current_page ());

	for (i = candidates.begin(); i != candidates.end(); ++i) {
		if (w == &(*i)->contents()) {
			if (prev != candidates.end()) {
				_tabs.set_current_page (_tabs.page_num ((*prev)->contents()));
			} else {
				_tabs.set_current_page (_tabs.page_num (candidates.back()->contents()));
			}
			return;
		}
		prev = i;
	}
}

void
ARDOUR_UI::step_down_through_tabs ()
{
	std::vector<Tabbable*> candidates;

	/* this list must match the order of visibility buttons */

	if (recorder->tabbed()) {
		candidates.push_back (recorder);
	}

	if (editor->tabbed()) {
		candidates.push_back (editor);
	}

	if (mixer->tabbed()) {
		candidates.push_back (mixer);
	}

	if (trigger_page->tabbed()) {
		candidates.push_back (trigger_page);
	}

	if (rc_option_editor->tabbed()) {
		candidates.push_back (rc_option_editor);
	}

	if (candidates.size() < 2) {
		/* nothing to be done with zero or one visible in tabs */
		return;
	}

	std::vector<Tabbable*>::reverse_iterator next = candidates.rend();
	std::vector<Tabbable*>::reverse_iterator i;
	Gtk::Widget* w = _tabs.get_nth_page (_tabs.get_current_page ());

	for (i = candidates.rbegin(); i != candidates.rend(); ++i) {
		if (w == &(*i)->contents()) {
			if (next != candidates.rend()) {
				_tabs.set_current_page (_tabs.page_num ((*next)->contents()));
			} else {
				_tabs.set_current_page (_tabs.page_num (candidates.front()->contents()));
			}
			break;
		}
		next = i;
	}
}

void
ARDOUR_UI::key_change_tabbable_visibility (Tabbable* t)
{
	if (!t) {
		return;
	}

	if (t->tabbed()) {
		_tabs.set_current_page (_tabs.page_num (t->contents()));
	} else if (!t->fully_visible()) {
		t->make_visible ();
	} else {
		_main_window.present ();
	}
}

void
ARDOUR_UI::button_change_tabbable_visibility (Tabbable* t)
{
	/* For many/most users, clicking a button in the main window will make it
	   the main/front/key window, which will change any stacking relationship they
	   were trying to modify by clicking on the button in the first
	   place. This button-aware method knows that click on
	   a button designed to show/hide a Tabbable that has its own window
	   will have made that window be obscured (as the main window comes to
	   the front). We therefore *hide* the Tabbable's window if it is even
	   partially visible, believing that this is likely because the
	   Tabbable window used to be front, the user clicked to change that,
	   and before we even get here, the main window has become front.
	*/

	if (!t) {
		return;
	}

	if (t->tabbed()) {
		_tabs.set_current_page (_tabs.page_num (t->contents()));
	} else if (t->visible()) {
		t->hide();
	} else {
		t->make_visible ();
	}
}

void
ARDOUR_UI::show_tabbable (Tabbable* t)
{
	if (!t) {
		return;
	}

	t->make_visible ();
}

void
ARDOUR_UI::hide_tabbable (Tabbable* t)
{
	if (!t) {
		return;
	}
	t->make_invisible ();
}

void
ARDOUR_UI::attach_tabbable (Tabbable* t)
{
	if (!t) {
		return;
	}

	t->attach ();
}

void
ARDOUR_UI::detach_tabbable (Tabbable* t)
{
	if (!t) {
		return;
	}
	t->detach ();
}

void
ARDOUR_UI::tabs_page_added (Widget*,guint)
{
	if (_tabs.get_n_pages() > 1) {

		std::vector<TargetEntry> drag_target_entries;
		drag_target_entries.push_back (TargetEntry ("ardour/x-tabbable"));

		editor_visibility_button.drag_source_set (drag_target_entries);
		mixer_visibility_button.drag_source_set (drag_target_entries);
		prefs_visibility_button.drag_source_set (drag_target_entries);
		recorder_visibility_button.drag_source_set (drag_target_entries);
		trigger_page_visibility_button.drag_source_set (drag_target_entries);

		editor_visibility_button.drag_source_set_icon (Gtkmm2ext::pixbuf_from_string (editor->name(),
		                                                                              Pango::FontDescription ("Sans 24"),
		                                                                              0, 0,
		                                                                              Gdk::Color ("red")));
		mixer_visibility_button.drag_source_set_icon (Gtkmm2ext::pixbuf_from_string (mixer->name(),
		                                                                             Pango::FontDescription ("Sans 24"),
		                                                                             0, 0,
		                                                                             Gdk::Color ("red")));
		prefs_visibility_button.drag_source_set_icon (Gtkmm2ext::pixbuf_from_string (rc_option_editor->name(),
		                                                                             Pango::FontDescription ("Sans 24"),
		                                                                             0, 0,
		                                                                             Gdk::Color ("red")));
		recorder_visibility_button.drag_source_set_icon (Gtkmm2ext::pixbuf_from_string (recorder->name(),
		                                                                                Pango::FontDescription ("Sans 24"),
		                                                                                0, 0,
		                                                                                Gdk::Color ("red")));
		trigger_page_visibility_button.drag_source_set_icon (Gtkmm2ext::pixbuf_from_string (recorder->name(),
		                                                                                    Pango::FontDescription ("Sans 24"),
		                                                                                    0, 0,
		                                                                                    Gdk::Color ("red")));
	}
}

void
ARDOUR_UI::tabs_page_removed (Widget*, guint)
{
	if (_tabs.get_n_pages() < 2) {
		editor_visibility_button.drag_source_unset ();
		mixer_visibility_button.drag_source_unset ();
		prefs_visibility_button.drag_source_unset ();
		recorder_visibility_button.drag_source_unset ();
		trigger_page_visibility_button.drag_source_unset ();
	}
}

void
ARDOUR_UI::tabs_switch (GtkNotebookPage*, guint page)
{
	if (tabbables_table.get_parent ()) {
		editor->tab_btn_box ().remove ();
		mixer->tab_btn_box ().remove ();
		recorder->tab_btn_box ().remove ();
		trigger_page->tab_btn_box ().remove ();
	}

	//pack the tabbables selector in this tab, and set button sensitivity appropriately
	if (editor && (page == (guint) _tabs.page_num (editor->contents()))) {

		editor->tab_btn_box ().add (tabbables_table);

		editor_visibility_button.set_active_state (Gtkmm2ext::ImplicitActive);

		if (mixer && (mixer->tabbed() || mixer->tabbed_by_default())) {
			mixer_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (rc_option_editor && (rc_option_editor->tabbed() || rc_option_editor->tabbed_by_default())) {
			prefs_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (recorder && (recorder->tabbed() || recorder->tabbed_by_default())) {
			recorder_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (trigger_page && (trigger_page->tabbed() || trigger_page->tabbed_by_default())) {
			trigger_page_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

	} else if (mixer && (page == (guint) _tabs.page_num (mixer->contents()))) {

		mixer->tab_btn_box ().add (tabbables_table);

		if (editor && (editor->tabbed() || editor->tabbed_by_default())) {
			editor_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		mixer_visibility_button.set_active_state (Gtkmm2ext::ImplicitActive);

		if (rc_option_editor && (rc_option_editor->tabbed() || rc_option_editor->tabbed_by_default())) {
			prefs_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (recorder && (recorder->tabbed() || recorder->tabbed_by_default())) {
			recorder_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (trigger_page && (trigger_page->tabbed() || trigger_page->tabbed_by_default())) {
			trigger_page_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

	} else if (page == (guint) _tabs.page_num (rc_option_editor->contents())) {

		if (editor && (editor->tabbed() || editor->tabbed_by_default())) {
			editor_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (mixer && (mixer->tabbed() || mixer->tabbed_by_default())) {
			mixer_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		prefs_visibility_button.set_active_state (Gtkmm2ext::ImplicitActive);

		if (recorder && (recorder->tabbed() || recorder->tabbed_by_default())) {
			recorder_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (trigger_page && (trigger_page->tabbed() || trigger_page->tabbed_by_default())) {
			trigger_page_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

	} else if (page == (guint) _tabs.page_num (recorder->contents())) {

		recorder->tab_btn_box ().add (tabbables_table);

		if (editor && (editor->tabbed() || editor->tabbed_by_default())) {
			editor_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (mixer && (mixer->tabbed() || mixer->tabbed_by_default())) {
			mixer_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (rc_option_editor && (rc_option_editor->tabbed() || rc_option_editor->tabbed_by_default())) {
			prefs_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		recorder_visibility_button.set_active_state (Gtkmm2ext::ImplicitActive);

		if (trigger_page && (trigger_page->tabbed() || trigger_page->tabbed_by_default())) {
			trigger_page_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

	} else if (page == (guint) _tabs.page_num (trigger_page->contents())) {

		trigger_page->tab_btn_box ().add (tabbables_table);

		if (editor && (editor->tabbed() || editor->tabbed_by_default())) {
			editor_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (mixer && (mixer->tabbed() || mixer->tabbed_by_default())) {
			mixer_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (rc_option_editor && (rc_option_editor->tabbed() || rc_option_editor->tabbed_by_default())) {
			prefs_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		if (recorder && (recorder->tabbed() || recorder->tabbed_by_default())) {
			recorder_visibility_button.set_active_state (Gtkmm2ext::Off);
		}

		trigger_page_visibility_button.set_active_state (Gtkmm2ext::ImplicitActive);

	}
}

void
ARDOUR_UI::tabbable_state_change (Tabbable& t)
{
	std::vector<std::string> insensitive_action_names;
	std::vector<std::string> sensitive_action_names;
	std::vector<std::string> active_action_names;
	std::vector<std::string> inactive_action_names;
	Glib::RefPtr<Action> action;

	enum ViewState {
		Tabbed,
		Windowed,
		Hidden
	};
	ViewState vs;

	if (t.tabbed()) {

		insensitive_action_names.push_back (string_compose ("attach-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("show-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("detach-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("hide-%1", t.menu_name()));

		vs = Tabbed;

	} else if (t.tabbed_by_default ()) {

		insensitive_action_names.push_back (string_compose ("attach-%1", t.menu_name()));
		insensitive_action_names.push_back (string_compose ("hide-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("show-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("detach-%1", t.menu_name()));

		vs = Hidden;

	} else if (t.window_visible()) {

		insensitive_action_names.push_back (string_compose ("detach-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("show-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("attach-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("hide-%1", t.menu_name()));

		active_action_names.push_back (string_compose ("show-%1", t.menu_name()));
		inactive_action_names.push_back (string_compose ("hide-%1", t.menu_name()));

		vs = Windowed;

	} else {

		/* not currently visible. allow user to retab it or just make
		 * it visible.
		 */

		insensitive_action_names.push_back (string_compose ("detach-%1", t.menu_name()));
		insensitive_action_names.push_back (string_compose ("hide-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("show-%1", t.menu_name()));
		sensitive_action_names.push_back (string_compose ("attach-%1", t.menu_name()));

		active_action_names.push_back (string_compose ("hide-%1", t.menu_name()));
		inactive_action_names.push_back (string_compose ("show-%1", t.menu_name()));

		vs = Hidden;
	}

	for (std::vector<std::string>::iterator s = insensitive_action_names.begin(); s != insensitive_action_names.end(); ++s) {
		action = ActionManager::get_action (X_("Common"), (*s).c_str(), false);
		if (action) {
			action->set_sensitive (false);
		}
	}

	for (std::vector<std::string>::iterator s = sensitive_action_names.begin(); s != sensitive_action_names.end(); ++s) {
		action = ActionManager::get_action (X_("Common"), (*s).c_str(), false);
		if (action) {
			action->set_sensitive (true);
		}
	}

	ArdourButton* vis_button = 0;

	if (&t == editor) {
		vis_button = &editor_visibility_button;
	} else if (&t == mixer) {
		vis_button = &mixer_visibility_button;
	} else if (&t == rc_option_editor) {
		vis_button = &prefs_visibility_button;
	} else if (&t == recorder) {
		vis_button = &recorder_visibility_button;
	} else if (&t == trigger_page) {
		vis_button = &trigger_page_visibility_button;
	}

	if (!vis_button) {
		assert (0);
		return;
	}

	/* First update button states for (other) tabbed windows.
	 * (Gtkmm2ext::Off or Gtkmm2ext::ImplicitActive)
	 */
	tabs_switch (NULL, _tabs.get_current_page ());

	switch (vs) {
		case Tabbed:
			/* nothing to do */
			break;
		case Windowed:
			vis_button->set_active_state (Gtkmm2ext::ExplicitActive);
			break;
		case Hidden:
			vis_button->set_active_state (Gtkmm2ext::Off);
			break;
	}
}

void
ARDOUR_UI::toggle_meterbridge ()
{
	assert (editor && mixer && meterbridge);

	bool show = false;
	bool obscuring = false;

	if (meterbridge->not_visible ()) {
		show = true;
	} else if ((editor->window_visible() && ARDOUR_UI_UTILS::windows_overlap (editor->own_window(), meterbridge)) ||
	           (mixer->window_visible () && ARDOUR_UI_UTILS::windows_overlap (mixer->own_window(), meterbridge))) {
		obscuring = true;
	}

	if (obscuring && ((editor->own_window() && editor->own_window()->property_has_toplevel_focus()) ||
	                  (mixer->own_window() && mixer->own_window()->property_has_toplevel_focus()))) {
		show = true;
	}

	if (show) {
		meterbridge->show_window ();
		meterbridge->present ();
		meterbridge->raise ();
	} else {
		meterbridge->hide_window (NULL);
	}
}

void
ARDOUR_UI::new_midi_tracer_window ()
{
	RefPtr<Action> act = ActionManager::get_action (X_("Common"), X_("NewMIDITracer"));
	if (!act) {
		return;
	}

	std::list<MidiTracer*>::iterator i = _midi_tracer_windows.begin ();
	while (i != _midi_tracer_windows.end() && (*i)->get_visible() == true) {
		++i;
	}

	if (i == _midi_tracer_windows.end()) {
		/* all our MIDITracer windows are visible; make a new one */
		MidiTracer* t = new MidiTracer ();
		t->show_all ();
		_midi_tracer_windows.push_back (t);
	} else {
		/* reuse the hidden one */
		(*i)->show_all ();
	}
}

KeyEditor*
ARDOUR_UI::create_key_editor ()
{
	KeyEditor* kedit = new KeyEditor;

	for (auto & b : Bindings::bindings) {
		if (!b->parent()) {
			kedit->add_tab (b->name(), *b);
		}
	}

	return kedit;
}

BundleManager*
ARDOUR_UI::create_bundle_manager ()
{
	return new BundleManager (_session);
}

AddVideoDialog*
ARDOUR_UI::create_add_video_dialog ()
{
	return new AddVideoDialog (_session);
}

SessionOptionEditor*
ARDOUR_UI::create_session_option_editor ()
{
	return new SessionOptionEditor (_session);
}

BigClockWindow*
ARDOUR_UI::create_big_clock_window ()
{
	return new BigClockWindow (*big_clock);
}

BigTransportWindow*
ARDOUR_UI::create_big_transport_window ()
{
	BigTransportWindow* btw = new BigTransportWindow ();
	return btw;
}

VirtualKeyboardWindow*
ARDOUR_UI::create_virtual_keyboard_window ()
{
	VirtualKeyboardWindow* vkbd = new VirtualKeyboardWindow ();
	return vkbd;
}

void
ARDOUR_UI::show_library_download_window ()
{
	library_download_window->show ();
}

LibraryDownloadDialog*
ARDOUR_UI::create_library_download_window ()
{
	LibraryDownloadDialog* ldd = new LibraryDownloadDialog ();
	return ldd;
}

LuaWindow*
ARDOUR_UI::create_luawindow ()
{
	LuaWindow* luawindow = LuaWindow::instance ();
	return luawindow;
}

RTAWindow*
ARDOUR_UI::create_rtawindow ()
{
	RTAWindow* rtawindow = new RTAWindow ();
	return rtawindow;
}

void
ARDOUR_UI::handle_locations_change (Location *)
{
	if (_session) {
		if (_session->locations()->num_range_markers()) {
			ActionManager::set_sensitive (ActionManager::range_sensitive_actions, true);
		} else {
			ActionManager::set_sensitive (ActionManager::range_sensitive_actions, false);
		}
	}
}

bool
ARDOUR_UI::tabbed_window_state_event_handler (GdkEventWindowState* ev, void* object)
{
#ifndef __APPLE__
	if (object == editor) {

		if ((ev->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) &&
		    (ev->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)) {
			if (big_clock_window) {
				big_clock_window->set_transient_for (*editor->own_window());
			}
			if (big_transport_window) {
				big_transport_window->set_transient_for (*editor->own_window());
			}
			if (virtual_keyboard_window) {
				virtual_keyboard_window->set_transient_for (*editor->own_window());
			}
		}

	} else if (object == mixer) {

		if ((ev->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) &&
		    (ev->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)) {
			if (big_clock_window) {
				big_clock_window->set_transient_for (*mixer->own_window());
			}
			if (big_transport_window) {
				big_transport_window->set_transient_for (*mixer->own_window());
			}
			if (virtual_keyboard_window) {
				virtual_keyboard_window->set_transient_for (*mixer->own_window());
			}
		}
	}
#endif

	return false;
}

void
ARDOUR_UI::toggle_mixer_space()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("Common", "ToggleMaximalMixer");
	if (tact->get_active()) {
		mixer->maximise_mixer_space ();
	} else {
		mixer->restore_mixer_space ();
	}
}
void
ARDOUR_UI::show_lua_window ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("Window", "toggle-luawindow");
	tact->set_active();
}

void
ARDOUR_UI::show_plugin_manager ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("Window", "toggle-plugin-manager");
	tact->set_active();
}

void
ARDOUR_UI::show_realtime_analyzer ()
{
	Glib::RefPtr<ToggleAction> tact = ActionManager::get_toggle_action ("Window", "toggle-rtawindow");
	tact->set_active();
}

bool
ARDOUR_UI::timecode_button_press (GdkEventButton* ev)
{
	if (ev->button != 1 || ev->type != GDK_2BUTTON_PRESS) {
		return false;
	}
	if (_session) {
		session_option_editor->show ();
		session_option_editor->set_current_page (_("Timecode"));
	}
	return true;
}

bool
ARDOUR_UI::format_button_press (GdkEventButton* ev)
{
	if (ev->button != 1 || ev->type != GDK_2BUTTON_PRESS) {
		return false;
	}
	if (_session) {
		session_option_editor->show ();
		session_option_editor->set_current_page (_("Media"));
	}
	return true;
}
