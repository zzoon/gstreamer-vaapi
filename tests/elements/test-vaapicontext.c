#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/vaapi/gstvaapivideocontext.h>
#include <gst/vaapi/gstvaapidisplay_x11.h>

typedef struct _CustomData
{
  GstElement *pipeline;
  GstElement *video_sink;
  GMainLoop *loop;
} AppData;

static void
set_rotation (AppData * data)
{
  static gint counter = 0;
  const static gint rotations[] = { 90, 180, 270, 0 };

  g_object_set (G_OBJECT (data->video_sink), "rotation",
      rotations[counter++ % G_N_ELEMENTS (rotations)], NULL);
}

/* Process keyboard input */
static gboolean
handle_keyboard (GIOChannel * source, GIOCondition cond, AppData * data)
{
  gchar *str = NULL;

  if (g_io_channel_read_line (source, &str, NULL, NULL,
          NULL) != G_IO_STATUS_NORMAL) {
    return TRUE;
  }

  switch (g_ascii_tolower (str[0])) {
    case 'r':
      set_rotation (data);
      break;
    case 'q':
      g_main_loop_quit (data->loop);
      break;
    default:
      break;
  }

  g_free (str);

  return TRUE;
}

static GstBusSyncReply
bus_sync_handler (GstBus * bus, GstMessage * msg, gpointer user_data)
{
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_NEED_CONTEXT:
    {
      const gchar *context_type;
      GstContext *context = NULL;

      gst_message_parse_context_type (msg, &context_type);
      g_print ("Got need context %s\n", context_type);

      if (g_strcmp0 (context_type, GST_VAAPI_DISPLAY_CONTEXT_TYPE_NAME) == 0) {
        Display *x11_display;
        GstVaapiDisplay *vaapi_display;

        x11_display = XOpenDisplay (NULL);
        if (x11_display == NULL) {
          g_print ("cannot open X11 display\n");
          return FALSE;
        }

        vaapi_display = GST_VAAPI_DISPLAY
            (gst_vaapi_display_x11_new_with_display (x11_display));

        context = gst_context_new (GST_VAAPI_DISPLAY_CONTEXT_TYPE_NAME, TRUE);
        gst_vaapi_video_context_set_display (context, vaapi_display);

        gst_element_set_context (GST_ELEMENT (msg->src), context);
      }
      if (context)
        gst_context_unref (context);
      break;
    }
    default:
      break;
  }

  return GST_BUS_PASS;
}

int
main (int argc, char *argv[])
{
  AppData data;
  GstStateChangeReturn ret;
  GIOChannel *io_stdin;
  GstBus *bus;

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Initialize our data structure */
  memset (&data, 0, sizeof (data));

  /* Print usage map */
  g_print ("USAGE: Choose one of the following options, then press enter:\n"
      " 'r' to set rotation value to vaapisink\n" " 'q' to quit\n");

  data.pipeline =
      gst_parse_launch
      ("videotestsrc name=src ! tee name=t t. ! queue ! vaapisink rotation=90 name=sink1 t. ! queue ! vaapisink name=sink2",
      NULL);

  data.video_sink = gst_bin_get_by_name (GST_BIN (data.pipeline), "sink1");

  bus = gst_element_get_bus (data.pipeline);
  gst_bus_set_sync_handler (bus, bus_sync_handler, (gpointer) & data, NULL);
  gst_object_unref (bus);

  /* Add a keyboard watch so we get notified of keystrokes */
  io_stdin = g_io_channel_unix_new (fileno (stdin));
  g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, &data);

  /* Start playing */
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Create a GLib Main Loop and set it to run */
  data.loop = g_main_loop_new (NULL, FALSE);

  g_main_loop_run (data.loop);

  /* Free resources */
  g_main_loop_unref (data.loop);
  g_io_channel_unref (io_stdin);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);

  if (data.video_sink != NULL)
    gst_object_unref (data.video_sink);
  gst_object_unref (data.pipeline);

  return 0;
}
