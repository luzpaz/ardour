// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_SIZEGROUP_H
#define _GTKMM_SIZEGROUP_H


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* $Id: sizegroup.hg,v 1.5 2006/12/11 18:57:50 murrayc Exp $ */

/* box.h
 *
 * Copyright 2002 The gtkmm Development Team
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

#include <glibmm/object.h>
#include <ytkmm/widget.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkSizeGroup GtkSizeGroup;
typedef struct _GtkSizeGroupClass GtkSizeGroupClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Gtk
{ class SizeGroup_Class; } // namespace Gtk
#endif //DOXYGEN_SHOULD_SKIP_THIS

namespace Gtk
{


/** @addtogroup gtkmmEnums gtkmm Enums and Flags */

/** 
 *
 * @ingroup gtkmmEnums
 */
enum SizeGroupMode
{
  SIZE_GROUP_NONE,
  SIZE_GROUP_HORIZONTAL,
  SIZE_GROUP_VERTICAL,
  SIZE_GROUP_BOTH
};

} // namespace Gtk


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Glib
{

template <>
class Value<Gtk::SizeGroupMode> : public Glib::Value_Enum<Gtk::SizeGroupMode>
{
public:
  static GType value_type() G_GNUC_CONST;
};

} // namespace Glib
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{


/** Gtk::SizeGroup provides a mechanism for grouping a number of widgets together so they all request the same amount of space. 
 * This is typically useful when you want a column of widgets to have the same size, but you can't use a Gtk::Table widget.
 * 
 * In detail, the size requested for each widget in a Gtk::SizeGroup is the maximum of the sizes that would have been 
 * requested for each widget in the size group if they were not in the size group. The mode of the size group (see 
 * set_mode()) determines whether this applies to the horizontal size, the vertical size, or both sizes.
 *
 * Note that size groups only affect the amount of space requested, not the size that the widgets finally receive. If 
 * you want the widgets in a GtkSizeGroup to actually be the same size, you need to pack them in such a way that they 
 * get the size they request and not more. For example, if you are packing your widgets into a table, you would not 
 * include the Gtk::FILL flag.
 * 
 * Widgets can be part of multiple size groups; GTK+ will compute the horizontal size of a widget from the horizontal 
 * requisition of all widgets that can be reached from the widget by a chain of size groups of type 
 * Gtk::SIZE_GROUP_HORIZONTAL or Gtk::SIZE_GROUP_BOTH, and the vertical size from the vertical requisition of all widgets  
 * that can be reached from the widget by a chain of size groups of type Gtk::SIZE_GROUP_VERTICAL or Gtk::SIZE_GROUP_BOTH.
 */

class SizeGroup : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef SizeGroup CppObjectType;
  typedef SizeGroup_Class CppClassType;
  typedef GtkSizeGroup BaseObjectType;
  typedef GtkSizeGroupClass BaseClassType;

private:  friend class SizeGroup_Class;
  static CppClassType sizegroup_class_;

private:
  // noncopyable
  SizeGroup(const SizeGroup&);
  SizeGroup& operator=(const SizeGroup&);

protected:
  explicit SizeGroup(const Glib::ConstructParams& construct_params);
  explicit SizeGroup(GtkSizeGroup* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~SizeGroup();

  /** Get the GType for this class, for use with the underlying GObject type system.
   */
  static GType get_type()      G_GNUC_CONST;

#ifndef DOXYGEN_SHOULD_SKIP_THIS


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GtkSizeGroup*       gobj()       { return reinterpret_cast<GtkSizeGroup*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GtkSizeGroup* gobj() const { return reinterpret_cast<GtkSizeGroup*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GtkSizeGroup* gobj_copy();

private:

protected:
  explicit SizeGroup(SizeGroupMode mode);

public:

  
  static Glib::RefPtr<SizeGroup> create(SizeGroupMode mode);


  /** Sets the Gtk::SizeGroupMode of the size group. The mode of the size
   * group determines whether the widgets in the size group should
   * all have the same horizontal requisition (Gtk::SIZE_GROUP_MODE_HORIZONTAL)
   * all have the same vertical requisition (Gtk::SIZE_GROUP_MODE_VERTICAL),
   * or should all have the same requisition in both directions
   * (Gtk::SIZE_GROUP_MODE_BOTH).
   * 
   * @param mode The mode to set for the size group.
   */
  void set_mode(SizeGroupMode mode);
  
  /** Gets the current mode of the size group. See set_mode().
   * 
   * @return The current mode of the size group.
   */
  SizeGroupMode get_mode() const;

  
  /** Sets whether unmapped widgets should be ignored when
   * calculating the size.
   * 
   * @param ignore_hidden Whether unmapped widgets should be ignored
   * when calculating the size.
   */
  void set_ignore_hidden(bool ignore_hidden =  true);
  
  /** Returns if invisible widgets are ignored when calculating the size.
   * 
   * @return <tt>true</tt> if invisible widgets are ignored.
   */
  bool get_ignore_hidden() const;

  
  /** Adds a widget to a Gtk::SizeGroup. In the future, the requisition
   * of the widget will be determined as the maximum of its requisition
   * and the requisition of the other widgets in the size group.
   * Whether this applies horizontally, vertically, or in both directions
   * depends on the mode of the size group. See set_mode().
   * 
   * When the widget is destroyed or no longer referenced elsewhere, it will 
   * be removed from the size group.
   * 
   * @param widget The Gtk::Widget to add.
   */
  void add_widget(Widget& widget);
  
  /** Removes a widget from a Gtk::SizeGroup.
   * 
   * @param widget The Gtk::Widget to remove.
   */
  void remove_widget(Widget& widget);

  
  /** Returns the list of widgets associated with @a size_group.
   * 
   * @return A SList of
   * widgets. The list is owned by GTK+ and should not be modified.
   */
  Glib::SListHandle<Widget*> get_widgets();
  
  /** Returns the list of widgets associated with @a size_group.
   * 
   * @return A SList of
   * widgets. The list is owned by GTK+ and should not be modified.
   */
  Glib::SListHandle<const Widget*> get_widgets() const;

  /** The directions in which the size group affects the requested sizes of its component widgets.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< SizeGroupMode > property_mode() ;

/** The directions in which the size group affects the requested sizes of its component widgets.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< SizeGroupMode > property_mode() const;

  /** If TRUE, unmapped widgets are ignored when determining the size of the group.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< bool > property_ignore_hidden() ;

/** If TRUE, unmapped widgets are ignored when determining the size of the group.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< bool > property_ignore_hidden() const;


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


};

} /* namespace Gtk */


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gtk::SizeGroup
   */
  Glib::RefPtr<Gtk::SizeGroup> wrap(GtkSizeGroup* object, bool take_copy = false);
}


#endif /* _GTKMM_SIZEGROUP_H */

