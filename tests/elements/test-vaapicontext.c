#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gtk/gtk.h>
#include "gst/vaapi/sysdeps.h"
#include <va/va.h>
#include <va/va_x11.h>

#if USE_X11
#include <X11/Xlib.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#else
#error "X11 is not supported in GTK+"
#endif
#endif

VADisplay g_va_display[2] = { NULL, };

static gint g_window_cnt;
static gchar *g_filepath;

static GOptionEntry g_options[] = {
  {"window", 'w',
        0,
        G_OPTION_ARG_INT, &g_window_cnt,
      "count of gtk window", NULL},
  {"file", 'f',
        0,
        G_OPTION_ARG_STRING, &g_filepath,
      "file path to play", NULL},
  {NULL,}
};


typedef struct _CustomData
{
  GtkWidget *video_widget;
  GstElement *pipeline;
  GstElement *dbin1;
  GstElement *dbin2;
  guintptr window_handle[2];
  guint video_widget_cnt;
} AppData;

static void
send_rotate_event (GstElement * elem, const gchar * orientation)
{
  gboolean res = FALSE;
  GstEvent *event;

  event = gst_event_new_tag (gst_tag_list_new (GST_TAG_IMAGE_ORIENTATION,
          orientation, NULL));

  /* Send the event */
  res = gst_element_send_event (elem, event);
  g_print ("Sending event %p done: %d\n", event, res);
}

static void
button_rotate_1_cb (GtkWidget * widget, GstElement * elem)
{
  static gint counter = 0;
  const static gchar *tags[] = { "rotate-90", "rotate-180", "rotate-270",
    "rotate-0"
  };

  send_rotate_event (elem, tags[counter++ % G_N_ELEMENTS (tags)]);
}

static void
button_rotate_2_cb (GtkWidget * widget, GstElement * elem)
{
  static gint counter_1 = 0;
  const static gchar *tags_1[] = { "rotate-90", "rotate-180", "rotate-270",
    "rotate-0"
  };

  send_rotate_event (elem, tags_1[counter_1++ % G_N_ELEMENTS (tags_1)]);
}

static void
quit_cb (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static gint
get_vaapi_index (GstElement * elem)
{
  GstElement *e = elem;

  if (!g_strcmp0 (GST_ELEMENT_NAME (elem), "sink1"))
    return 0;

  else if (!g_strcmp0 (GST_ELEMENT_NAME (elem), "sink2"))
    return 1;

  while ((e = GST_ELEMENT (gst_element_get_parent (e)))) {
    if (!g_strcmp0 (GST_ELEMENT_NAME (e), "dbin1"))
      return 0;
    else if (!g_strcmp0 (GST_ELEMENT_NAME (e), "dbin2"))
      return 1;
  }

  return -1;
}

static GstBusSyncReply
bus_sync_handler (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  AppData *data = (AppData *) user_data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_NEED_CONTEXT:
    {
      const gchar *context_type;
      GstContext *context = NULL;
      GstStructure *s;

      gst_message_parse_context_type (msg, &context_type);
      g_print ("Got need context %s from %s\n", context_type,
          GST_ELEMENT_NAME (msg->src));

#if USE_X11
      if (g_strcmp0 (context_type, "gst.va.display.handle") == 0) {
        GdkDisplay *gdk_display = gtk_widget_get_display (data->video_widget);
        Display *x11_display;
        VADisplay va_display = NULL;
        gint vaapi_idx;

        x11_display = gdk_x11_display_get_xdisplay (gdk_display);
        if (x11_display == NULL) {
          g_print ("cannot open X11 display\n");
          return FALSE;
        }

        vaapi_idx = get_vaapi_index (GST_ELEMENT (msg->src));
        if (vaapi_idx == -1)
          break;

        if (!g_va_display[vaapi_idx])
          g_va_display[vaapi_idx] = vaGetDisplay (x11_display);

        va_display = g_va_display[vaapi_idx];

        context = gst_context_new ("gst.va.display.handle", TRUE);

        s = gst_context_writable_structure (context);
        gst_structure_set (s, "display", G_TYPE_POINTER, va_display, NULL);
        gst_structure_set (s, "x11-display", G_TYPE_POINTER, x11_display, NULL);

        gst_element_set_context (GST_ELEMENT (msg->src), context);
      }
#endif

      if (context)
        gst_context_unref (context);
      break;
    }
    case GST_MESSAGE_ELEMENT:
    {
      gint vaapi_idx;
      if (!gst_is_video_overlay_prepare_window_handle_message (msg))
        break;
      vaapi_idx = get_vaapi_index (GST_ELEMENT (msg->src));

      gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (GST_MESSAGE_SRC
              (msg)), data->window_handle[vaapi_idx]);
      break;
    }
    case GST_MESSAGE_EOS:
      gtk_main_quit ();
      break;
    default:
      break;
  }

  return GST_BUS_PASS;
}

static void
realize_cb (GtkWidget * widget, AppData * data)
{
  GdkWindow *window = gtk_widget_get_window (widget);

  if (!gdk_window_ensure_native (window))
    g_error ("Couldn't create native window needed for GstXOverlay!");

#if defined(USE_X11) && defined(GDK_WINDOWING_X11)
  data->video_widget = widget;
  data->window_handle[data->video_widget_cnt] = GDK_WINDOW_XID (window);
  data->video_widget_cnt++;
#endif
}

int
main (int argc, char *argv[])
{
  AppData app;
  GstBus *bus;

  GtkWidget *windowHead;
  GtkWidget *video_window;
  GtkWidget *video_window2;
  GtkWidget *pane;
  GtkWidget *button_quit;
  GtkWidget *button_rotate_2;
  GtkWidget *button_rotate_1;
  GtkWidget *video_box_1;
  GtkWidget *video_box_2;
  GtkWidget *button_box_1;
  GtkWidget *button_box_2;

  GOptionContext *ctx;
  GError *error = NULL;
  gchar *pipeline_str = NULL;

  XInitThreads ();
  gst_init (NULL, NULL);
  gtk_init (NULL, NULL);

  memset (&app, 0x00, sizeof (app));

  ctx = g_option_context_new ("- test options");
  if (!ctx)
    return -1;

  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_main_entries (ctx, g_options, NULL);
  g_option_context_parse (ctx, &argc, &argv, NULL);
  g_option_context_free (ctx);

  if (g_window_cnt < 1 || g_window_cnt > 2)
    return -1;

#define FILESRC "filesrc location=%s"

  if (g_window_cnt == 1) {
    if (!g_filepath) {
      app.pipeline =
          gst_parse_launch
          ("videotestsrc ! vaapih264enc ! vaapidecodebin name=dbin1 ! vaapisink name=sink1 rotation=360",
          &error);
      app.dbin1 = gst_bin_get_by_name (GST_BIN (app.pipeline), "dbin1");
    } else {
      pipeline_str =
          g_strdup_printf (FILESRC
          " ! decodebin name=dbin1 ! queue name=q ! vaapisink name=sink1 rotation=360",
          g_filepath);
      app.pipeline = gst_parse_launch (pipeline_str, &error);
      app.dbin1 = gst_bin_get_by_name (GST_BIN (app.pipeline), "q");
    }
  } else if (g_window_cnt == 2) {
    if (!g_filepath) {
      app.pipeline =
          gst_parse_launch
          ("videotestsrc ! vaapih264enc ! tee name=t ! vaapidecodebin name=dbin1 ! vaapisink name=sink1 rotation=360 "
          " t. ! vaapidecodebin name=dbin2 ! vaapisink name=sink2 rotation=360",
          &error);
    } else {
      pipeline_str =
          g_strdup_printf (FILESRC
          " ! tee name=t ! queue ! decodebin name=dbin1 ! vaapisink name=sink1 rotation=360 "
          "t. ! queue ! decodebin name=dbin2 ! vaapisink name=sink2 rotation=360",
          g_filepath);
      app.pipeline = gst_parse_launch (pipeline_str, &error);
    }
    app.dbin1 = gst_bin_get_by_name (GST_BIN (app.pipeline), "dbin1");
    app.dbin2 = gst_bin_get_by_name (GST_BIN (app.pipeline), "dbin2");
  }
  g_free (pipeline_str);

  windowHead = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  video_box_1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  button_box_1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  pane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

  video_window = gtk_drawing_area_new ();
  gtk_widget_set_size_request (video_window, 320, 240);
  g_signal_connect (video_window, "realize", G_CALLBACK (realize_cb), &app);

  button_quit = gtk_button_new_with_label ("Quit");
  button_rotate_1 = gtk_button_new_with_label ("Rotate");

  g_signal_connect (G_OBJECT (button_rotate_1), "clicked",
      G_CALLBACK (button_rotate_1_cb), app.dbin1);
  g_signal_connect (G_OBJECT (button_quit), "clicked", G_CALLBACK (quit_cb),
      &app);

  gtk_box_pack_start (GTK_BOX (button_box_1), button_rotate_1, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (button_box_1), button_quit, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (video_box_1), video_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (video_box_1), button_box_1, TRUE, TRUE, 0);

  gtk_paned_pack1 (GTK_PANED (pane), video_box_1, TRUE, FALSE);

  if (g_window_cnt == 2) {
    video_box_2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    button_box_2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    video_window2 = gtk_drawing_area_new ();
    gtk_widget_set_size_request (video_window2, 320, 240);
    g_signal_connect (video_window2, "realize", G_CALLBACK (realize_cb), &app);

    button_rotate_2 = gtk_button_new_with_label ("Rotate");
    g_signal_connect (G_OBJECT (button_rotate_2), "clicked",
        G_CALLBACK (button_rotate_2_cb), app.dbin2);

    gtk_box_pack_start (GTK_BOX (button_box_2), button_rotate_2, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (video_box_2), video_window2, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (video_box_2), button_box_2, TRUE, TRUE, 0);

    gtk_paned_pack2 (GTK_PANED (pane), video_box_2, TRUE, FALSE);
  }

  gtk_container_add (GTK_CONTAINER (windowHead), pane);

  if (g_window_cnt == 1)
    gtk_window_set_default_size (GTK_WINDOW (windowHead), 400, 300);
  else
    gtk_window_set_default_size (GTK_WINDOW (windowHead), 800, 300);

  gtk_widget_show_all (windowHead);

  bus = gst_element_get_bus (app.pipeline);
  gst_bus_set_sync_handler (bus, bus_sync_handler, (gpointer) & app, NULL);
  gst_object_unref (bus);

  gst_element_set_state (app.pipeline, GST_STATE_PLAYING);

  g_print ("Now playing...\n");

  gtk_main ();

  gst_element_set_state (app.pipeline, GST_STATE_NULL);
  gst_object_unref (app.dbin1);
  if (app.dbin2)
    gst_object_unref (app.dbin2);
  gst_object_unref (app.pipeline);

  return 0;
}
