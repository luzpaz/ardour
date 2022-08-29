/*
 * Copyright (C) 2016-2018 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2016-2018 Robin Gareus <robin@gareus.org>
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

#include <cairomm/region.h>
#include <pangomm/layout.h>

#include "pbd/compose.h"
#include "pbd/convert.h"
#include "pbd/debug.h"
#include "pbd/failed_constructor.h"
#include "pbd/file_utils.h"
#include "pbd/search_path.h"
#include "pbd/enumwriter.h"

#include "midi++/parser.h"

#include "temporal/time.h"
#include "temporal/bbt_time.h"

#include "ardour/async_midi_port.h"
#include "ardour/audioengine.h"
#include "ardour/debug.h"
#include "ardour/dsp_filter.h"
#include "ardour/filesystem_paths.h"
#include "ardour/midiport_manager.h"
#include "ardour/midi_track.h"
#include "ardour/midi_port.h"
#include "ardour/monitor_control.h"
#include "ardour/meter.h"
#include "ardour/selection.h"
#include "ardour/send.h"
#include "ardour/session.h"
#include "ardour/solo_isolate_control.h"
#include "ardour/solo_safe_control.h"
#include "ardour/tempo.h"
#include "ardour/triggerbox.h"

#include "gtkmm2ext/gui_thread.h"
#include "gtkmm2ext/rgb_macros.h"

#include "canvas/arc.h"
#include "canvas/box.h"
#include "canvas/line.h"
#include "canvas/meter.h"
#include "canvas/rectangle.h"
#include "canvas/text.h"
#include "canvas/types.h"

#include "canvas.h"
#include "cues.h"
#include "knob.h"
#include "level_meter.h"
#include "push2.h"
#include "utils.h"

#include "pbd/i18n.h"

#ifdef __APPLE__
#define Rect ArdourCanvas::Rect
#endif

using namespace ARDOUR;
using namespace PBD;
using namespace Glib;
using namespace ArdourSurface;
using namespace ArdourCanvas;
using namespace Gtkmm2ext;

CueLayout::CueLayout (Push2& p, Session & s, std::string const & name)
	: Push2Layout (p, s, name)
	, track_base (0)
	, scene_base (0)
	, _knob_function (KnobGain)
	, _long_stop (0)
{
	Pango::FontDescription fd ("Sans 10");

	_bg = new ArdourCanvas::Rectangle (this);
	_bg->set (Rect (0, 0, display_width(), display_height()));
	_bg->set_fill_color (_p2.get_color (Push2::DarkBackground));

	_upper_line = new Line (this);
	_upper_line->set (Duple (0, 22.5), Duple (display_width(), 22.5));
	_upper_line->set_outline_color (_p2.get_color (Push2::LightBackground));

	for (int n = 0; n < 8; ++n) {
		Text* t;

		/* background for text labels for knob function */

		ArdourCanvas::Rectangle* r = new ArdourCanvas::Rectangle (this);
		Coord x0 = 10 + (n*Push2Canvas::inter_button_spacing()) - 5;
		r->set (Rect (x0, 2, x0 + Push2Canvas::inter_button_spacing(), 2 + 21));
		_upper_backgrounds.push_back (r);

		t = new Text (this);
		t->set_font_description (fd);
		t->set_color (_p2.get_color (Push2::ParameterName));
		t->set_position ( Duple (10 + (n*Push2Canvas::inter_button_spacing()), 2));
		_upper_text.push_back (t);

		switch (n) {
		case 0: t->set (_("Gain")); break;
		case 1: t->set (_("Pan")); break;
		case 2: t->set (_("Send A")); break;
		case 3: t->set (_("Send B")); break;
		default:
			break;
		}

		t = new Text (this);
		t->set_font_description (fd);
		t->set_color (_p2.get_color (Push2::ParameterName));
		t->set_position (Duple (10 + (n*Push2Canvas::inter_button_spacing()), 140));

		_lower_text.push_back (t);

		_progress[n] = new Arc (this);
		_progress[n]->set_position (Duple (45 + (Push2Canvas::inter_button_spacing()*n), 80));
		_progress[n]->set_radius (25.);
		_progress[n]->set_start (-90.); /* 0 is "east" */
		_progress[n]->set_fill_color (_p2.get_color (Push2::KnobForeground));
		_progress[n]->set_fill (false);
		_progress[n]->set_outline_color (_p2.get_color (Push2::KnobArcBackground));
		_progress[n]->set_outline_width (10.);
		_progress[n]->set_outline (true);

		t = new Text (this);
		t->set_font_description (fd);
		t->set_color (_p2.get_color (Push2::ParameterName));
		t->set_position ( Duple (10 + (n*Push2Canvas::inter_button_spacing()), 115));
		_clip_label_text.push_back (t);
	}
}

CueLayout::~CueLayout ()
{
}

void
CueLayout::show ()
{
	Push2::ButtonID lower_buttons[] = {
		Push2::Lower1, Push2::Lower2, Push2::Lower3, Push2::Lower4,
		Push2::Lower5, Push2::Lower6, Push2::Lower7, Push2::Lower8
	};

	for (auto & lb : lower_buttons) {
		boost::shared_ptr<Push2::Button> b = _p2.button_by_id (lb);
		b->set_color (Push2::LED::DarkGray);
		b->set_state (Push2::LED::OneShot24th);
		_p2.write (b->state_msg());
	}

	Push2::ButtonID scene_buttons[] = {
		Push2::Fwd32ndT, Push2::Fwd32nd, Push2::Fwd16th, Push2::Fwd16thT,
		Push2::Fwd8thT, Push2::Fwd8th, Push2::Fwd4trT, Push2::Fwd4tr
	};

	for (auto & sb : scene_buttons) {
		boost::shared_ptr<Push2::Button> b = _p2.button_by_id (sb);
		b->set_color (Push2::LED::Green);
		b->set_state (Push2::LED::NoTransition);
		_p2.write (b->state_msg());
	}

	show_state ();
	viewport_changed ();
	show_knob_function ();

	Container::show ();
}

void
CueLayout::hide ()
{
	Push2::ButtonID scene_buttons[] = {
		Push2::Fwd32ndT, Push2::Fwd32nd, Push2::Fwd16th, Push2::Fwd16thT,
		Push2::Fwd8thT, Push2::Fwd8th, Push2::Fwd4trT, Push2::Fwd4tr
	};

	for (auto & sb : scene_buttons) {
		boost::shared_ptr<Push2::Button> b = _p2.button_by_id (sb);
		b->set_color (Push2::LED::Black);
		b->set_state (Push2::LED::NoTransition);
		_p2.write (b->state_msg());
	}
}

void
CueLayout::render (Rect const & area, Cairo::RefPtr<Cairo::Context> context) const
{
	Container::render (area, context);
}

void
CueLayout::button_upper (uint32_t n)
{
	switch (n) {
	case 0:
		_knob_function = KnobGain;
		break;
	case 1:
		_knob_function = KnobPan;
		break;
	case 2:
		_knob_function = KnobSendA;
		break;
	case 3:
		_knob_function = KnobSendB;
		break;
	default:
		return;
	}

	show_knob_function ();
	viewport_changed ();
}

void
CueLayout::show_knob_function ()
{
	for (int s = 0; s < 8; ++s) {
		_upper_backgrounds[s]->hide ();
		_upper_text[s]->set_color (_p2.get_color (Push2::ParameterName));
	}

	int n = 0;

	switch (_knob_function)  {
	case KnobGain:
		break;
	case KnobPan:
		n = 1;
		break;
	case KnobSendA:
		n = 2;
		break;
	case KnobSendB:
		n = 3;
		break;
	default:
		return;
	}
	_upper_backgrounds[n]->set_fill_color (_p2.get_color (Push2::ParameterName));
	_upper_backgrounds[n]->set_outline_color (_p2.get_color (Push2::ParameterName));
	_upper_backgrounds[n]->show ();
	_upper_text[n]->set_color (Gtkmm2ext::contrasting_text_color (_p2.get_color (Push2::ParameterName)));

}

void
CueLayout::button_lower (uint32_t n)
{
	if (_p2.stop_down() || _long_stop) {
		std::cerr << "stop trigger in " << n + track_base << std::endl;
		_p2.unbang (n + track_base);
	} else {
		/* select track */

		boost::shared_ptr<Route> r = _session.get_remote_nth_route (n);

		if (r) {
			_session.selection().set (r, boost::shared_ptr<AutomationControl>());
		}
	}
}

void
CueLayout::button_left ()
{
	if (track_base > 0) {
		track_base--;
		viewport_changed ();
		show_state ();
	}
}

void
CueLayout::button_page_left ()
{
	if (track_base > 8) {
		track_base -= 8; /* XXX get back to zero when appropriate */
		viewport_changed ();
		show_state ();
	}
}

void
CueLayout::button_right ()
{
	track_base++;
	viewport_changed ();
	show_state ();
}

void
CueLayout::button_page_right ()
{
	track_base += 8; /* XXX limit to number of tracks */
	viewport_changed ();
	show_state ();
}

void
CueLayout::button_up ()
{
	if (scene_base > 0) {
		scene_base--;
		viewport_changed ();
		show_state ();
	}
}

void
CueLayout::button_octave_up ()
{
	if (scene_base > 8) {
		scene_base -= 8;
		viewport_changed ();
		show_state ();
	}
}

void
CueLayout::button_down ()
{
	scene_base++;
	viewport_changed ();
	show_state ();
}

void
CueLayout::button_octave_down ()
{
	scene_base++;
	show_state ();
}

void
CueLayout::viewport_changed ()
{
	_route_connections.drop_connections ();

	for (int n = 0; n < 64; ++n) {
		_trig_connections[n].disconnect ();
	}

	for (int n = 0; n < 8; ++n) {

		_route[n] = _session.get_remote_nth_route (track_base+n);

		boost::shared_ptr<Route> r = _route[n];

		boost::shared_ptr<Push2::Button> lower_button = _p2.lower_button_by_column (n);

		if (r) {
			_route[n]->DropReferences.connect (_route_connections, invalidator (*this), boost::bind (&CueLayout::viewport_changed, this), &_p2);
			_route[n]->presentation_info().PropertyChanged.connect (_route_connections, invalidator (*this), boost::bind (&CueLayout::route_property_change, this, _1, n), &_p2);

			std::string shortname = short_version (r->name(), 10);
			_lower_text[n]->set (shortname);

			boost::shared_ptr<Processor> s;

			switch (_knob_function) {
			case KnobGain:
				_controllables[n] = r->gain_control();
				break;
			case KnobPan:
				_controllables[n] = r->pan_azimuth_control ();
				break;
			case KnobSendA:
				s = r->nth_send (0);
				if (s) {
					boost::shared_ptr<Send> ss = boost::dynamic_pointer_cast<Send> (s);
					if (ss) {
						_controllables[n] = ss->gain_control();
					} else {
						_controllables[n] = boost::shared_ptr<AutomationControl> ();
					}
				}
				break;
			case KnobSendB:
				s = r->nth_send (1);
				if (s) {
					boost::shared_ptr<Send> ss = boost::dynamic_pointer_cast<Send> (s);
					if (ss) {
						_controllables[n] = ss->gain_control();
					} else {
						_controllables[n] = boost::shared_ptr<AutomationControl> ();
					}
				}
				break;
			default:
				_controllables[n] = boost::shared_ptr<AutomationControl> ();
			}

			uint8_t color = _p2.get_color_index (r->presentation_info().color());

			lower_button->set_color (color);
			lower_button->set_state (Push2::LED::OneShot24th);
			_p2.write (lower_button->state_msg());

			boost::shared_ptr<TriggerBox> tb = r->triggerbox ();

			if (tb) {
				tb->PropertyChanged.connect (_route_connections, invalidator (*this), boost::bind (&CueLayout::triggerbox_property_change, this, _1, n), &_p2);
			}

			for (int y = 0; y < 8; ++y) {
				boost::shared_ptr<Push2::Pad> pad = _p2.pad_by_xy (n, y);
				if (tb && tb->active()) {
					TriggerPtr tp = tb->trigger (y);
					if (tp && tp->region()) {
						/* trigger in slot */
						pad->set_color (color);

						tp->PropertyChanged.connect (_trig_connections[n * 8 + y], invalidator (*this), boost::bind (&CueLayout::trigger_property_change, this, _1, n, y), &_p2);

					} else {
						/* no trigger */
						pad->set_color (Push2::LED::Black);
					}
				} else {
					/* no active triggerbox */
					pad->set_color (Push2::LED::Black);
				}
				pad->set_state (Push2::LED::OneShot24th);
				_p2.write (pad->state_msg());

			}

		} else {

			_lower_text[n]->set (std::string());
			lower_button->set_color (Push2::LED::Black);
			_controllables[n] = boost::shared_ptr<AutomationControl> ();

			/* turn this column off */

			for (int y = 0; y < 8; ++y) {
				boost::shared_ptr<Push2::Pad> pad = _p2.pad_by_xy (n, y);
				pad->set_color (Push2::LED::Black);
				pad->set_state (Push2::LED::OneShot24th);
				_p2.write (pad->state_msg());
			}
		}
	}
}

void
CueLayout::show_state ()
{
	if (!parent()) {
		return;
	}
}

void
CueLayout::strip_vpot (int n, int delta)
{
	boost::shared_ptr<Controllable> ac = _controllables[n];

	if (ac) {
		ac->set_value (ac->get_value() + ((2.0/64.0) * delta), PBD::Controllable::UseGroup);
	}
}

void
CueLayout::strip_vpot_touch (int n, bool touching)
{
}

void
CueLayout::button_rhs (int row)
{
	_p2.get_session().cue_bang (row + scene_base);
}

void
CueLayout::button_stop_press ()
{
	if (_p2.modifier_state() == Push2::ModShift) {
		_p2.get_session().stop_all_triggers (false); /* quantized global stop */
	}
}
void
CueLayout::button_stop_release ()
{
	std::cerr << "BS release, ls = " << _long_stop << std::endl;
	if (_long_stop) {
		_long_stop = 0;
		show_running_boxen (false);
	}
}

void
CueLayout::button_stop_long_press ()
{
	_long_stop++;

	if (_long_stop == 1) {
		show_running_boxen (true);
	}
}

void
CueLayout::show_running_boxen (bool yn)
{
	Push2::ButtonID lower_buttons[] = {
		Push2::Lower1, Push2::Lower2, Push2::Lower3, Push2::Lower4,
		Push2::Lower5, Push2::Lower6, Push2::Lower7, Push2::Lower8
	};

	for (int n = 0; n < 8; ++n) {
		boost::shared_ptr<Push2::Button> lower_button = _p2.button_by_id (lower_buttons[n]);

		if (!_route[n]) {
			continue;
		}

		boost::shared_ptr<TriggerBox> tb = _route[n]->triggerbox();
		assert (tb);

		if (yn) {

			if (!tb->currently_playing()) {
				/* nothing playing, do not turn the blink on */
				continue;
			}

			HSV hsv (_route[n]->presentation_info().color());
			hsv = hsv.shade (2.0);
			lower_button->set_color (_p2.get_color_index (hsv.color()));
			lower_button->set_state (Push2::LED::Blinking4th);

		} else {
			std::cerr << "no blink " << n << std::endl;
			lower_button->set_color (_p2.get_color_index (_route[n]->presentation_info().color()));
			lower_button->set_state (Push2::LED::NoTransition);
		}

		_p2.write (lower_button->state_msg());
	}
}

void
CueLayout::pad_press (int y, int x) /* fix coordinate order one day */
{
	std::cerr << "bang on " << x + track_base << ", " << y + scene_base << std::endl;

	if (!_route[x]) {
		std::cerr << "no route\n";
		return;
	}

	boost::shared_ptr<TriggerBox> tb = _route[x]->triggerbox();

	if (!tb) {
		std::cerr << "no TB\n";
		/* unpossible! */
		return;
	}

	if (!tb->trigger (y + scene_base)->region()) {
		std::cerr << "empty slot, unbang TB\n";
		_p2.unbang (x + track_base);
		return;
	}

	std::cerr << "bang TB\n";
	_p2.bang (x + track_base, y + scene_base);



}

void
CueLayout::update_meters ()
{
	for (int n = 0; n < 8; ++n) {
		update_clip_progress (n);
	}
}

void
CueLayout::update_clip_progress (int n)
{
	boost::shared_ptr<Route> r = _p2.get_session().get_remote_nth_route (n + track_base);
	if (!r) {
		_progress[n]->set_arc (0.0 - 90.0);
		return;
	}
	boost::shared_ptr<TriggerBox> tb = r->triggerbox();

	if (!tb || !tb->active()) {
		_progress[n]->set_arc (0.0 - 90.0);
		_clip_label_text[n]->set (std::string());
		return;
	}

	double fract = tb->position_as_fraction ();
	if (fract < 0.0) {
		_progress[n]->set_arc (0.0 - 90.0); /* 0 degrees is "east" */
	} else {
		_progress[n]->set_arc ((fract * 360.0) - 90.0); /* 0 degrees is "east" */
	}

	TriggerPtr tp = tb->currently_playing();
	if (tp) {
		std::string shortname = short_version (tp->name(), 10);
		_clip_label_text[n]->set (shortname);
	} else {
		_clip_label_text[n]->set (std::string());
	}
}

void
CueLayout::route_property_change (PropertyChange const& what_changed, uint32_t which)
{
	if (what_changed.contains (Properties::color)) {
		// _lower_backgrounds[which]->set_fill_color (_stripable[which]->presentation_info().color());

		if (_route[which]->is_selected()) {
			_lower_text[which]->set_fill_color (contrasting_text_color (_route[which]->presentation_info().color()));
			/* might not be a MIDI track, in which case this will
			   do nothing
			*/
			_p2.update_selection_color ();
		}
	}

	if (what_changed.contains (Properties::hidden)) {
		viewport_changed ();
	}

	if (what_changed.contains (Properties::selected)) {

		if (!_route[which]) {
			return;
		}

		if (_route[which]->is_selected()) {
			// show_selection (which);
		} else {
			// hide_selection (which);
		}
	}

}

void
CueLayout::trigger_property_change (PropertyChange const& what_changed, uint32_t col, uint32_t row)
{
	assert (_route[col]);

	if (what_changed.contains (Properties::running)) {
		boost::shared_ptr<TriggerBox> tb = _route[col]->triggerbox ();
		assert (tb);

		TriggerPtr trig = tb->trigger (row);
		assert (trig);

		boost::shared_ptr<Push2::Pad> pad = _p2.pad_by_xy (col, row);
		assert (pad);

		set_pad_color_from_trigger_state (col, pad, trig);
		_p2.write (pad->state_msg());
	}
}

void
CueLayout::triggerbox_property_change (PropertyChange const& what_changed, uint32_t col)
{
	std::cerr << "TB prop change "; what_changed.dump (std::cerr); std::cerr << std::endl;

	assert (_route[col]);

	if (what_changed.contains (Properties::currently_playing) || what_changed.contains (Properties::queued)) {

		boost::shared_ptr<TriggerBox> tb = _route[col]->triggerbox ();
		assert (tb);


		/* make sure the blink state of all 8 pads for this
		 * route/triggerbox are correct
		 */

		for (uint32_t y = 0; y < 8; ++y) {
			boost::shared_ptr<Push2::Pad> pad = _p2.pad_by_xy (col, y);
			assert (pad);

			TriggerPtr trig = tb->trigger (y);
			assert (trig);

			set_pad_color_from_trigger_state (col, pad, trig);

			_p2.write (pad->state_msg());
		}


		if (!what_changed.contains (Properties::queued)) {

			/* currently_playing changed, if nothing is playing be
			 * sure to disable blink on lower button
			 */

			std::cerr << "running change on " << col << std::endl;

			if (!tb->currently_playing()) {
				boost::shared_ptr<Push2::Button> lower_button = _p2.lower_button_by_column (col);
				lower_button->set_color (_p2.get_color_index (_route[col]->presentation_info().color()));
				lower_button->set_state (Push2::LED::NoTransition);
				_p2.write (lower_button->state_msg());
			}
		}
	}
}

void
CueLayout::set_pad_color_from_trigger_state (int col, boost::shared_ptr<Push2::Pad> pad, TriggerPtr trig)
{
	if (trig->region()) {

		if (trig->active()) {

			/* running or waiting to stop */

			HSV hsv (_route[col]->presentation_info().color());
			hsv = hsv.shade (2.0);
			pad->set_color (_p2.get_color_index (hsv.color ()));
			pad->set_state (Push2::LED::Pulsing4th);

		} else if (trig == trig->box().peek_next_trigger()) {

			/* waiting to start */

			HSV hsv (_route[col]->presentation_info().color());
			hsv = hsv.shade (2.0);
			pad->set_color (_p2.get_color_index (hsv.color ()));
			pad->set_state (Push2::LED::Pulsing16th);

		} else {

			/* not running */

			pad->set_color (_p2.get_color_index (_route[col]->presentation_info().color()));
			pad->set_state (Push2::LED::NoTransition);
		}

	} else {

		/* empty slot */
		pad->set_color (Push2::LED::Black);
		pad->set_state (Push2::LED::NoTransition);
	}
}