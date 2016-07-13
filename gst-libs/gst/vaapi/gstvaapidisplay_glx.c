/*
 *  gstvaapidisplay_glx.c - VA/GLX display abstraction
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2013 Intel Corporation
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

/**
 * SECTION:gstvaapidisplay_glx
 * @short_description: VA/GLX display abstraction
 */

#include "sysdeps.h"
#include "gstvaapicompat.h"
#include "gstvaapiutils.h"
#include "gstvaapiutils_glx.h"
#include "gstvaapidisplay.h"
#include "gstvaapidisplay_x11.h"
#include "gstvaapidisplay_glx.h"
#include "gstvaapiwindow_glx.h"
#include "gstvaapitexture_glx.h"

#define DEBUG 1
#include "gstvaapidebug.h"

G_DEFINE_TYPE (GstVaapiDisplayGLX, gst_vaapi_display_glx,
    GST_TYPE_VAAPI_DISPLAY);

static GstVaapiWindow *
gst_vaapi_display_glx_create_window (GstVaapiDisplay * display, GstVaapiID id,
    guint width, guint height)
{
  return id != GST_VAAPI_ID_INVALID ?
      gst_vaapi_window_glx_new_with_xid (display, id) :
      gst_vaapi_window_glx_new (display, width, height);
}

static GstVaapiTexture *
gst_vaapi_display_glx_create_texture (GstVaapiDisplay * display, GstVaapiID id,
    guint target, guint format, guint width, guint height)
{
  return id != GST_VAAPI_ID_INVALID ?
      gst_vaapi_texture_glx_new_wrapped (display, id, target, format) :
      gst_vaapi_texture_glx_new (display, target, format, width, height);
}

static void
gst_vaapi_display_glx_init (GstVaapiDisplayGLX * display)
{
}

static void
gst_vaapi_display_glx_class_init (GstVaapiDisplayGLXClass * klass)
{
  GstVaapiDisplayClass *const dpy_class = GST_VAAPI_DISPLAY_CLASS (klass);

  dpy_class->display_type = GST_VAAPI_DISPLAY_TYPE_GLX;
  dpy_class->create_window = gst_vaapi_display_glx_create_window;
  dpy_class->create_texture = gst_vaapi_display_glx_create_texture;
}

/**
 * gst_vaapi_display_glx_new:
 * @display_name: the X11 display name
 *
 * Opens an X11 #Display using @display_name and returns a newly
 * allocated #GstVaapiDisplay object. The X11 display will be cloed
 * when the reference count of the object reaches zero.
 *
 * Return value: a newly allocated #GstVaapiDisplay object
 */
GstVaapiDisplayGLX *
gst_vaapi_display_glx_new (const gchar * display_name)
{
  return (GstVaapiDisplayGLX *)
      gst_vaapi_display_new (GST_TYPE_VAAPI_DISPLAY_GLX,
      GST_VAAPI_DISPLAY_INIT_FROM_DISPLAY_NAME, (gpointer) display_name);
}

/**
 * gst_vaapi_display_glx_new_with_display:
 * @x11_display: an X11 #Display
 *
 * Creates a #GstVaapiDisplay based on the X11 @x11_display
 * display. The caller still owns the display and must call
 * XCloseDisplay() when all #GstVaapiDisplay references are
 * released. Doing so too early can yield undefined behaviour.
 *
 * Return value: a newly allocated #GstVaapiDisplay object
 */
GstVaapiDisplayGLX *
gst_vaapi_display_glx_new_with_display (Display * x11_display)
{
  g_return_val_if_fail (x11_display != NULL, NULL);

  return (GstVaapiDisplayGLX *)
      gst_vaapi_display_new (GST_TYPE_VAAPI_DISPLAY_GLX,
      GST_VAAPI_DISPLAY_INIT_FROM_NATIVE_DISPLAY, x11_display);
}
