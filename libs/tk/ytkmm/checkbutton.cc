// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!


#include <glibmm.h>

#include <ytkmm/checkbutton.h>
#include <ytkmm/private/checkbutton_p.h>


// -*- c++ -*-
/* $Id: checkbutton.ccg,v 1.1 2003/01/21 13:38:44 murrayc Exp $ */

/* 
 *
 * Copyright 1998-2002 The gtkmm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <ytk/ytk.h>

namespace Gtk
{

CheckButton::CheckButton(const Glib::ustring& label, bool mnemonic)
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::ToggleButton(Glib::ConstructParams(checkbutton_class_.init(), "label",label.c_str(),"use_underline",gboolean(mnemonic), static_cast<char*>(0)))
{}

} // namespace Gtk


namespace
{
} // anonymous namespace


namespace Glib
{

Gtk::CheckButton* wrap(GtkCheckButton* object, bool take_copy)
{
  return dynamic_cast<Gtk::CheckButton *> (Glib::wrap_auto ((GObject*)(object), take_copy));
}

} /* namespace Glib */

namespace Gtk
{


/* The *_Class implementation: */

const Glib::Class& CheckButton_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Class has to know the class init function to clone custom types.
    class_init_func_ = &CheckButton_Class::class_init_function;

    // This is actually just optimized away, apparently with no harm.
    // Make sure that the parent type has been created.
    //CppClassParent::CppObjectType::get_type();

    // Create the wrapper type, with the same class/instance size as the base type.
    register_derived_type(gtk_check_button_get_type());

    // Add derived versions of interfaces, if the C type implements any interfaces:

  }

  return *this;
}


void CheckButton_Class::class_init_function(void* g_class, void* class_data)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
  CppClassParent::class_init_function(klass, class_data);

  klass->draw_indicator = &draw_indicator_vfunc_callback;

}

void CheckButton_Class::draw_indicator_vfunc_callback(GtkCheckButton* self, GdkRectangle* area)
{
  Glib::ObjectBase *const obj_base = static_cast<Glib::ObjectBase*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj_base && obj_base->is_derived_())
  {
    CppObjectType *const obj = dynamic_cast<CppObjectType* const>(obj_base);
    if(obj) // This can be NULL during destruction.
    {
      try // Trap C++ exceptions which would normally be lost because this is a C callback.
      {
        // Call the virtual member method, which derived classes might override.
        obj->draw_indicator_vfunc(area);
        return;
      }
      catch(...)
      {
        Glib::exception_handlers_invoke();
      }
    }
  }

  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(self)) // Get the parent class of the object class (The original underlying C class).
  );

  // Call the original underlying C function:
  if(base && base->draw_indicator)
  {
    (*base->draw_indicator)(self, area);
  }

}


Glib::ObjectBase* CheckButton_Class::wrap_new(GObject* o)
{
  return manage(new CheckButton((GtkCheckButton*)(o)));

}


/* The implementation: */

CheckButton::CheckButton(const Glib::ConstructParams& construct_params)
:
  Gtk::ToggleButton(construct_params)
{
  }

CheckButton::CheckButton(GtkCheckButton* castitem)
:
  Gtk::ToggleButton((GtkToggleButton*)(castitem))
{
  }

CheckButton::~CheckButton()
{
  destroy_();
}

CheckButton::CppClassType CheckButton::checkbutton_class_; // initialize static member

GType CheckButton::get_type()
{
  return checkbutton_class_.init().get_type();
}


GType CheckButton::get_base_type()
{
  return gtk_check_button_get_type();
}


CheckButton::CheckButton()
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Gtk::ToggleButton(Glib::ConstructParams(checkbutton_class_.init()))
{
  

}


void Gtk::CheckButton::draw_indicator_vfunc(GdkRectangle* area) 
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->draw_indicator)
  {
    (*base->draw_indicator)(gobj(),area);
  }
}


} // namespace Gtk


