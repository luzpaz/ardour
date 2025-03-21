/*
 * Copyright (C) 2011-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2014-2017 Robin Gareus <robin@gareus.org>
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

#pragma once

#include <memory>

#include <ytkmm/drawingarea.h>
#include <ytkmm/label.h>
#include "gtkmm2ext/persistent_tooltip.h"

#include "pbd/destructible.h"

namespace PBD {
	class Controllable;
}

namespace ARDOUR {
	class Panner;
}

class PannerEditor;

class PannerPersistentTooltip : public Gtkmm2ext::PersistentTooltip
{
public:
	PannerPersistentTooltip (Gtk::Widget* w);

	void target_start_drag ();
	void target_stop_drag ();

	bool dragging () const;

private:
	bool _dragging;
};


/** Parent class for some panner UI classes that contains some common code */
class PannerInterface : public Gtk::DrawingArea, public PBD::Destructible
{
public:
	PannerInterface (std::shared_ptr<ARDOUR::Panner>);
	virtual ~PannerInterface ();

	std::shared_ptr<ARDOUR::Panner> panner () {
		return _panner;
	}

	void edit ();
	void set_send_drawing_mode (bool);

protected:
	virtual void set_tooltip () = 0;
	virtual std::weak_ptr<PBD::Controllable> proxy_controllable () const = 0;

	void value_change ();

	bool on_enter_notify_event (GdkEventCrossing *);
	bool on_leave_notify_event (GdkEventCrossing *);
	bool on_key_release_event  (GdkEventKey *);
	bool on_button_press_event (GdkEventButton*);
	bool on_button_release_event (GdkEventButton*);

	std::shared_ptr<ARDOUR::Panner> _panner;
	PannerPersistentTooltip _tooltip;

	bool _send_mode;

private:
	virtual PannerEditor* editor () = 0;
	PannerEditor* _editor;
};

