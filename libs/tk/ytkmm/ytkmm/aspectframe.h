// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_ASPECTFRAME_H
#define _GTKMM_ASPECTFRAME_H


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* $Id: aspectframe.hg,v 1.2 2003/03/03 07:49:05 murrayc Exp $ */

/* aspectframe.h
 * 
 * Copyright (C) 1998-2002 The gtkmm Development Team
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

#include <ytkmm/frame.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkAspectFrame GtkAspectFrame;
typedef struct _GtkAspectFrameClass GtkAspectFrameClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class AspectFrame_Class; } // namespace Gtk
namespace Gtk
{

/** A frame that constrains its child to a particular aspect ratio.
 *
 * The Gtk::AspectFrame is useful when you want pack a widget so that it can
 * resize but always retains the same aspect ratio.  For instance, one might
 * be drawing a small preview of a larger image. Because it derives from
 * Gtk::Frame, it can draw a label and a frame around the child. The frame
 * will be "shrink-wrapped" to the size of the child.
 *
 * @ingroup Widgets
 * @ingroup Containers
 */

class AspectFrame : public Frame
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef AspectFrame CppObjectType;
  typedef AspectFrame_Class CppClassType;
  typedef GtkAspectFrame BaseObjectType;
  typedef GtkAspectFrameClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~AspectFrame();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class AspectFrame_Class;
  static CppClassType aspectframe_class_;

  // noncopyable
  AspectFrame(const AspectFrame&);
  AspectFrame& operator=(const AspectFrame&);

protected:
  explicit AspectFrame(const Glib::ConstructParams& construct_params);
  explicit AspectFrame(GtkAspectFrame* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GtkAspectFrame*       gobj()       { return reinterpret_cast<GtkAspectFrame*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GtkAspectFrame* gobj() const { return reinterpret_cast<GtkAspectFrame*>(gobject_); }


public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


private:


public:

  /** Constructor to create a new AspectFrame object.
   * @param label Label text.
   * @param xalign Horizontal alignment of the child within the allocation of this
   * AspectFrame.  For possible values, see Gtk::AlignmentEnum.
   * @param yalign Vertical alignment of the child within the allocation of this 
   * AspectFrame. For possible values, see Gtk::AlignmentEnum.
   * @param ratio The desired aspect ratio.
   * @param obey_child If <tt>true</tt>, @a ratio  is ignored and the aspect ratio
   * is taken from the requisition of the child.
   */
    explicit AspectFrame(const Glib::ustring& label =  Glib::ustring(), AlignmentEnum xalign =  Gtk::ALIGN_CENTER, AlignmentEnum yalign =  Gtk::ALIGN_CENTER, float ratio =  1.0, bool obey_child =  false);


  /** Constructor to create a new AspectFrame object.
   * @param label Label text.
   * @param xalign Horizontal alignment of the child within the allocation of this 
   * AspectFrame.  This ranges from 0.0 (left aligned) to 1.0 (right aligned).
   * @param yalign Vertical alignment of the child within the allocation of this 
   * AspectFrame. This ranges from 0.0 (top aligned) to 1.0 (bottom aligned).
   * @param ratio The desired aspect ratio.
   * @param obey_child If <tt>true</tt>, @a ratio  is ignored and the aspect ratio 
   * is taken from the requisition of the child.
   */
    explicit AspectFrame(const Glib::ustring& label, float xalign, float yalign, float ratio =  1.0, bool obey_child =  false);


  //TODO: Is "obey_child = true" a good default?
  
  /** Set the properties of this AspectFrame.
   * @param xalign Horizontal alignment of the child within the allocation of this 
   * AspectFrame.  This ranges from 0.0 (left aligned) to 1.0 (right aligned).
   * @param yalign Vertical alignment of the child within the allocation of this 
   * AspectFrame.  This ranges from 0.0 (top aligned) to 1.0 (bottom aligned).
   * @param ratio The desired aspect ratio.
   * @param obey_child If <tt>true</tt>, @a ratio  is ignored and the aspect ratio 
   * is taken from the requisition of the child.
   */
  void set(float xalign, float yalign, float ratio, bool obey_child =  true);

  
  /** Set the properties of this AspectFrame.
   * @param xalign Horizontal alignment of the child within the allocation of this 
   * AspectFrame.  For possible values, see Gtk::AlignmentEnum.
   * @param yalign Vertical alignment of the child within the allocation of this 
   * AspectFrame.  For possible values, see Gtk::AlignmentEnum.
   * @param ratio The desired aspect ratio.
   * @param obey_child If <tt>true</tt>, @a ratio  is ignored and the aspect ratio
   * is taken from the requisition of the child.
   */
  void set(AlignmentEnum xalign, AlignmentEnum yalign, float ratio, bool obey_child =  true);

  /** X alignment of the child.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< float > property_xalign() ;

/** X alignment of the child.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< float > property_xalign() const;

  /** Y alignment of the child.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< float > property_yalign() ;

/** Y alignment of the child.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< float > property_yalign() const;

  /** Aspect ratio if obey_child is FALSE.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< float > property_ratio() ;

/** Aspect ratio if obey_child is FALSE.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< float > property_ratio() const;

  /** Force aspect ratio to match that of the frame's child.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< bool > property_obey_child() ;

/** Force aspect ratio to match that of the frame's child.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< bool > property_obey_child() const;


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
   * @relates Gtk::AspectFrame
   */
  Gtk::AspectFrame* wrap(GtkAspectFrame* object, bool take_copy = false);
} //namespace Glib


#endif /* _GTKMM_ASPECTFRAME_H */

