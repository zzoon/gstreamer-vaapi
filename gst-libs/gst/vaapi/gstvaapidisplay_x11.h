/*
 *  gstvaapidisplay_x11.h - VA/X11 display abstraction
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2012-2013 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef GST_VAAPI_DISPLAY_X11_H
#define GST_VAAPI_DISPLAY_X11_H

#include <va/va_x11.h>
#include <gst/vaapi/gstvaapidisplay.h>

G_BEGIN_DECLS

typedef struct _GstVaapiDisplayX11              GstVaapiDisplayX11;
typedef struct _GstVaapiDisplayX11Class         GstVaapiDisplayX11Class;
typedef struct _GstVaapiDisplayX11Private       GstVaapiDisplayX11Private;

GType gst_vaapi_display_x11_get_type (void);

#define GST_TYPE_VAAPI_DISPLAY_X11            (gst_vaapi_display_x11_get_type ())
#define GST_VAAPI_DISPLAY_X11(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_VAAPI_DISPLAY_X11, GstVaapiDisplayX11))
#define GST_VAAPI_DISPLAY_X11_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_VAAPI_DISPLAY_X11, GstVaapiDisplayX11Class))
#define GST_IS_VAAPI_DISPLAY_X11(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_VAAPI_DISPLAY_X11))
#define GST_IS_VAAPI_DISPLAY_X11_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_VAAPI_DISPLAY_X11))
#define GST_VAAPI_DISPLAY_X11_CAST(obj)           ((GstVaapiDisplayX11*)(obj))
#define GST_VAAPI_DISPLAY_X11_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_VAAPI_DISPLAY_X11, GstVaapiDisplayX11Class))

/**
 * GstVaapiDisplayX11:
 *
 * VA/X11 display wrapper.
 */
struct _GstVaapiDisplayX11
{
  /*< private >*/
  GstVaapiDisplay parent_instance;

  GstVaapiDisplayX11Private *priv;
};

/**
 * GstVaapiDisplayX11Class:
 *
 * VA/X11 display wrapper clas.
 */
struct _GstVaapiDisplayX11Class
{
  /*< private >*/
  GstVaapiDisplayClass parent_class;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GST_VAAPI_DISPLAY_XDISPLAY:
 * @display: a #GstVaapiDisplay
 *
 * Macro that evaluates to the underlying X11 #Display of @display
 */
#undef  GST_VAAPI_DISPLAY_XDISPLAY
#define GST_VAAPI_DISPLAY_XDISPLAY(display) \
    gst_vaapi_display_x11_get_xdisplay (display)

/**
 * GST_VAAPI_DISPLAY_XSCREEN:
 * @display: a #GstVaapiDisplay
 *
 * Macro that evaluates to the underlying X11 screen of @display
 */
#undef  GST_VAAPI_DISPLAY_XSCREEN
#define GST_VAAPI_DISPLAY_XSCREEN(display) \
    gst_vaapi_display_x11_get_xscreen (display)

/**
 * GST_VAAPI_DISPLAY_HAS_XRENDER:
 * @display: a #GstVaapiDisplay
 *
 * Macro that evaluates to the existence of the XRender extension on
 * @display server.
 */
#undef  GST_VAAPI_DISPLAY_HAS_XRENDER
#define GST_VAAPI_DISPLAY_HAS_XRENDER(display) \
    gst_vaapi_display_x11_has_xrender (display)

Display *
gst_vaapi_display_x11_get_xdisplay (GstVaapiDisplayX11 * display);

int
gst_vaapi_display_x11_get_xscreen (GstVaapiDisplayX11 * display);

gboolean
gst_vaapi_display_x11_has_xrender (GstVaapiDisplay * display);

GstVaapiDisplayX11 *
gst_vaapi_display_x11_new (const gchar * display_name);

GstVaapiDisplayX11 *
gst_vaapi_display_x11_new_with_display (Display * x11_display);

Display *
gst_vaapi_display_x11_get_display (GstVaapiDisplayX11 * display);

int
gst_vaapi_display_x11_get_screen (GstVaapiDisplayX11 * display);

void
gst_vaapi_display_x11_set_synchronous (GstVaapiDisplayX11 * display,
    gboolean synchronous);

//G_GNUC_INTERNAL
GstVideoFormat
gst_vaapi_display_x11_get_pixmap_format (GstVaapiDisplayX11 * display,
    guint depth);

//G_GNUC_INTERNAL
guint
gst_vaapi_display_x11_get_pixmap_depth (GstVaapiDisplayX11 * display,
    GstVideoFormat format);

G_END_DECLS

#endif /* GST_VAAPI_DISPLAY_X11_H */
