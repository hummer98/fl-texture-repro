#include "include/fl_texture_repro/fl_texture_repro_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

#include <cstring>

// ============================================================================
// GStreamer + FlTextureGL implementation to reproduce rendering artifact issue
// Uses videotestsrc instead of camera to allow testing without hardware
// ============================================================================

// Forward declarations
G_DECLARE_FINAL_TYPE(GstGLTexture, gst_gl_texture, GST, GL_TEXTURE, FlTextureGL)

struct _GstGLTexture {
  FlTextureGL parent_instance;

  // OpenGL
  GLuint texture_id;
  gboolean texture_initialized;

  // GStreamer
  GstElement* pipeline;
  GstElement* appsink;

  // Frame data
  uint8_t* frame_buffer;
  uint32_t width;
  uint32_t height;
  GMutex mutex;
  gboolean has_new_frame;
};

G_DEFINE_TYPE(GstGLTexture, gst_gl_texture, fl_texture_gl_get_type())

// GStreamer new sample callback
static GstFlowReturn on_new_sample(GstAppSink* appsink, gpointer user_data) {
  GstGLTexture* self = GST_GL_TEXTURE(user_data);

  GstSample* sample = gst_app_sink_pull_sample(appsink);
  if (sample == nullptr) {
    return GST_FLOW_OK;
  }

  GstBuffer* buffer = gst_sample_get_buffer(sample);
  GstCaps* caps = gst_sample_get_caps(sample);

  GstVideoInfo video_info;
  if (!gst_video_info_from_caps(&video_info, caps)) {
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  GstMapInfo map;
  if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
    g_mutex_lock(&self->mutex);

    uint32_t new_width = GST_VIDEO_INFO_WIDTH(&video_info);
    uint32_t new_height = GST_VIDEO_INFO_HEIGHT(&video_info);
    size_t buffer_size = new_width * new_height * 4;  // RGBA

    // Reallocate if size changed
    if (self->width != new_width || self->height != new_height) {
      g_free(self->frame_buffer);
      self->frame_buffer = (uint8_t*)g_malloc(buffer_size);
      self->width = new_width;
      self->height = new_height;
    }

    // Copy frame data
    memcpy(self->frame_buffer, map.data, buffer_size);
    self->has_new_frame = TRUE;

    g_mutex_unlock(&self->mutex);
    gst_buffer_unmap(buffer, &map);
  }

  gst_sample_unref(sample);
  return GST_FLOW_OK;
}

// Populate callback - called by Flutter to get texture
static gboolean gst_gl_texture_populate(FlTextureGL* texture,
                                        uint32_t* target,
                                        uint32_t* name,
                                        uint32_t* width,
                                        uint32_t* height,
                                        GError** error) {
  GstGLTexture* self = GST_GL_TEXTURE(texture);

  g_mutex_lock(&self->mutex);

  if (self->frame_buffer == nullptr || self->width == 0 || self->height == 0) {
    g_mutex_unlock(&self->mutex);
    return FALSE;
  }

  // Create texture if not initialized
  if (!self->texture_initialized) {
    glGenTextures(1, &self->texture_id);
    self->texture_initialized = TRUE;
  }

  // Bind texture - NOTE: We intentionally do NOT save/restore GL state
  // This is to reproduce the bug where GL state pollution causes artifacts
  glBindTexture(GL_TEXTURE_2D, self->texture_id);

  // Upload frame data to texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               self->width, self->height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, self->frame_buffer);

  // Set texture parameters - these also modify GL state
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  self->has_new_frame = FALSE;

  *target = GL_TEXTURE_2D;
  *name = self->texture_id;
  *width = self->width;
  *height = self->height;

  g_mutex_unlock(&self->mutex);

  // NOTE: We do NOT unbind or restore previous texture binding here.
  // This is intentional to reproduce the issue where Skia's GL state
  // is corrupted by external texture operations.

  return TRUE;
}

static gboolean gst_gl_texture_start_pipeline(GstGLTexture* self) {
  // Initialize GStreamer if needed
  if (!gst_is_initialized()) {
    gst_init(nullptr, nullptr);
  }

  // Create pipeline with v4l2src for real camera
  // Using Insta360 X5 connected via USB (supports 1920x1080 @ 30fps MJPEG)
  const char* pipeline_str =
      "v4l2src device=/dev/video0 ! "
      "image/jpeg,width=1920,height=1080,framerate=30/1 ! "
      "jpegdec ! "
      "videoscale ! "
      "video/x-raw,width=640,height=480 ! "
      "videoconvert ! "
      "video/x-raw,format=RGBA ! "
      "appsink name=sink emit-signals=true max-buffers=2 drop=true";

  GError* error = nullptr;
  self->pipeline = gst_parse_launch(pipeline_str, &error);

  if (error != nullptr) {
    g_warning("Failed to create pipeline: %s", error->message);
    g_error_free(error);
    return FALSE;
  }

  // Get appsink and configure callbacks
  self->appsink = gst_bin_get_by_name(GST_BIN(self->pipeline), "sink");
  if (self->appsink == nullptr) {
    g_warning("Failed to get appsink");
    gst_object_unref(self->pipeline);
    self->pipeline = nullptr;
    return FALSE;
  }

  // Set up callbacks
  GstAppSinkCallbacks callbacks = {
      nullptr,  // eos
      nullptr,  // new_preroll
      on_new_sample,  // new_sample
  };
  gst_app_sink_set_callbacks(GST_APP_SINK(self->appsink), &callbacks, self, nullptr);

  // Start pipeline
  GstStateChangeReturn ret = gst_element_set_state(self->pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_warning("Failed to start pipeline");
    gst_object_unref(self->appsink);
    gst_object_unref(self->pipeline);
    self->appsink = nullptr;
    self->pipeline = nullptr;
    return FALSE;
  }

  g_print("GStreamer pipeline started (v4l2src /dev/video0 640x480 @ 30fps)\n");
  return TRUE;
}

static void gst_gl_texture_stop_pipeline(GstGLTexture* self) {
  if (self->pipeline != nullptr) {
    gst_element_set_state(self->pipeline, GST_STATE_NULL);

    if (self->appsink != nullptr) {
      gst_object_unref(self->appsink);
      self->appsink = nullptr;
    }

    gst_object_unref(self->pipeline);
    self->pipeline = nullptr;

    g_print("GStreamer pipeline stopped\n");
  }
}

static void gst_gl_texture_dispose(GObject* object) {
  GstGLTexture* self = GST_GL_TEXTURE(object);

  gst_gl_texture_stop_pipeline(self);

  g_mutex_lock(&self->mutex);

  // Note: We don't call glDeleteTextures here because:
  // 1. The GL context may not be current
  // 2. Flutter's texture registrar will handle texture cleanup
  // Just reset our tracking variables
  self->texture_id = 0;
  self->texture_initialized = FALSE;

  g_free(self->frame_buffer);
  self->frame_buffer = nullptr;

  g_mutex_unlock(&self->mutex);
  g_mutex_clear(&self->mutex);

  G_OBJECT_CLASS(gst_gl_texture_parent_class)->dispose(object);
}

static void gst_gl_texture_class_init(GstGLTextureClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = gst_gl_texture_dispose;
  FL_TEXTURE_GL_CLASS(klass)->populate = gst_gl_texture_populate;
}

static void gst_gl_texture_init(GstGLTexture* self) {
  self->texture_id = 0;
  self->texture_initialized = FALSE;
  self->pipeline = nullptr;
  self->appsink = nullptr;
  self->frame_buffer = nullptr;
  self->width = 0;
  self->height = 0;
  self->has_new_frame = FALSE;
  g_mutex_init(&self->mutex);
}

static GstGLTexture* gst_gl_texture_new() {
  return GST_GL_TEXTURE(g_object_new(gst_gl_texture_get_type(), nullptr));
}

// ============================================================================
// Plugin implementation
// ============================================================================

#define FL_TEXTURE_REPRO_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), fl_texture_repro_plugin_get_type(), \
                              FlTextureReproPlugin))

struct _FlTextureReproPlugin {
  GObject parent_instance;
  FlTextureRegistrar* texture_registrar;
  GstGLTexture* texture;
  int64_t texture_id;
  guint timer_id;
};

G_DEFINE_TYPE(FlTextureReproPlugin, fl_texture_repro_plugin, g_object_get_type())

// Timer callback to notify Flutter of new frames
static gboolean update_texture_callback(gpointer user_data) {
  FlTextureReproPlugin* self = FL_TEXTURE_REPRO_PLUGIN(user_data);

  if (self->texture != nullptr && self->texture->has_new_frame) {
    fl_texture_registrar_mark_texture_frame_available(
        self->texture_registrar, FL_TEXTURE(self->texture));
  }

  return G_SOURCE_CONTINUE;
}

static void fl_texture_repro_plugin_handle_method_call(
    FlTextureReproPlugin* self,
    FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;
  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "initialize") == 0) {
    // Create texture
    self->texture = gst_gl_texture_new();

    // Register with Flutter
    gboolean registered = fl_texture_registrar_register_texture(
        self->texture_registrar, FL_TEXTURE(self->texture));

    if (!registered) {
      g_object_unref(self->texture);
      self->texture = nullptr;
      response = FL_METHOD_RESPONSE(
          fl_method_error_response_new("TEXTURE_ERROR", "Failed to register texture", nullptr));
      fl_method_call_respond(method_call, response, nullptr);
      return;
    }

    self->texture_id = fl_texture_get_id(FL_TEXTURE(self->texture));

    // Start GStreamer pipeline
    if (!gst_gl_texture_start_pipeline(self->texture)) {
      fl_texture_registrar_unregister_texture(
          self->texture_registrar, FL_TEXTURE(self->texture));
      g_object_unref(self->texture);
      self->texture = nullptr;
      response = FL_METHOD_RESPONSE(
          fl_method_error_response_new("PIPELINE_ERROR", "Failed to start GStreamer pipeline", nullptr));
      fl_method_call_respond(method_call, response, nullptr);
      return;
    }

    // Start timer to check for new frames (~60Hz to ensure we catch all frames)
    self->timer_id = g_timeout_add(16, update_texture_callback, self);

    g_autoptr(FlValue) result = fl_value_new_int(self->texture_id);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));

  } else if (strcmp(method, "dispose") == 0) {
    if (self->timer_id != 0) {
      g_source_remove(self->timer_id);
      self->timer_id = 0;
    }

    if (self->texture != nullptr) {
      gst_gl_texture_stop_pipeline(self->texture);
      fl_texture_registrar_unregister_texture(
          self->texture_registrar, FL_TEXTURE(self->texture));
      g_object_unref(self->texture);
      self->texture = nullptr;
    }

    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));

  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void fl_texture_repro_plugin_dispose(GObject* object) {
  FlTextureReproPlugin* self = FL_TEXTURE_REPRO_PLUGIN(object);

  if (self->timer_id != 0) {
    g_source_remove(self->timer_id);
    self->timer_id = 0;
  }

  if (self->texture != nullptr) {
    gst_gl_texture_stop_pipeline(self->texture);
    g_object_unref(self->texture);
    self->texture = nullptr;
  }

  G_OBJECT_CLASS(fl_texture_repro_plugin_parent_class)->dispose(object);
}

static void fl_texture_repro_plugin_class_init(FlTextureReproPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = fl_texture_repro_plugin_dispose;
}

static void fl_texture_repro_plugin_init(FlTextureReproPlugin* self) {
  self->texture_registrar = nullptr;
  self->texture = nullptr;
  self->texture_id = -1;
  self->timer_id = 0;
}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  FlTextureReproPlugin* plugin = FL_TEXTURE_REPRO_PLUGIN(user_data);
  fl_texture_repro_plugin_handle_method_call(plugin, method_call);
}

void fl_texture_repro_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  FlTextureReproPlugin* plugin = FL_TEXTURE_REPRO_PLUGIN(
      g_object_new(fl_texture_repro_plugin_get_type(), nullptr));

  // Store texture registrar
  plugin->texture_registrar = fl_plugin_registrar_get_texture_registrar(registrar);

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "fl_texture_repro",
                            FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
