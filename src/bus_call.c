/*
https://github.com/NVIDIA-AI-IOT/redaction_with_deepstream/blob/master/deepstream_redaction_app.c
*/

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <time.h>
#include "gstnvdsmeta.h"

GstElement *pipeline, *video_full_processing_bin;
gint frame_number = 0;
gchar pgie_classes_str[4][32] = { "face", "license_plate", "make", "model" };

gchar *pgie_config = NULL;
gchar *input_mp4 = NULL;
gchar *output_mp4 = NULL;
gchar *output_kitti = NULL;

/* Define global variables:
 * `frame_number` & `pgie_classes_str` are used for writing meta to kitti;
 * `pgie_config`,`input_mp4`,`output_mp4`,`output_kitti` are configurable file paths parsed through command line. */
clock_t t_start; 
clock_t t_end;

/* This bus callback function detects the error and state change in the main pipe 
 * and then export messages, export pipeline images or terminate the pipeline accordingly. */
static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      t_end = clock(); 
      clock_t t = t_end - t_start;
      double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
      double fps = frame_number/time_taken;
      g_print("\nThe program took %.2f seconds to redact %d frames, pref = %.2f fps \n\n", time_taken,frame_number,fps); 
      
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      g_free (debug);
      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:{
      GstState oldstate, newstate;
      gst_message_parse_state_changed (msg, &oldstate, &newstate, NULL);
      if (GST_ELEMENT (GST_MESSAGE_SRC (msg)) == pipeline) {
        switch (newstate) {
          case GST_STATE_PLAYING:
            g_print ("Pipeline running\n");
            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline),
                GST_DEBUG_GRAPH_SHOW_ALL, "ds-app-playing");
            t_start = clock(); 
            break;
          case GST_STATE_PAUSED:
            if (oldstate == GST_STATE_PLAYING) {
              g_print ("Pipeline paused\n");
            }
            break;
          case GST_STATE_READY:
            // GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (pipeline), GST_DEBUG_GRAPH_SHOW_ALL,"ds-app-ready");
            if (oldstate == GST_STATE_NULL) {
              g_print ("Pipeline ready\n");
            } else {
              g_print ("Pipeline stopped\n");
            }
            break;
          default:
            break;
        }
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
}
