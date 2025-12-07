#include "include/fl_texture_repro/fl_texture_repro_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <epoxy/gl.h>

#include <cstring>

// ============================================================================
// Simple OpenGL Texture implementation using FlTextureGL
// This is a minimal implementation to reproduce the rendering artifact issue
// ============================================================================

// Forward declarations
G_DECLARE_FINAL_TYPE(SimpleGLTexture, simple_gl_texture, SIMPLE, GL_TEXTURE, FlTextureGL)

struct _SimpleGLTexture {
  FlTextureGL parent_instance;
  GLuint texture_id;
  uint32_t width;
  uint32_t height;
  gboolean initialized;
  uint8_t color_offset;  // For simple animation
};

G_DEFINE_TYPE(SimpleGLTexture, simple_gl_texture, fl_texture_gl_get_type())

// Populate callback - this is where the texture is created/updated
static gboolean simple_gl_texture_populate(FlTextureGL* texture,
                                           uint32_t* target,
                                           uint32_t* name,
                                           uint32_t* width,
                                           uint32_t* height,
                                           GError** error) {
  SimpleGLTexture* self = SIMPLE_GL_TEXTURE(texture);

  if (!self->initialized) {
    // Create texture
    glGenTextures(1, &self->texture_id);
    self->initialized = TRUE;
  }

  // Bind and update texture
  glBindTexture(GL_TEXTURE_2D, self->texture_id);

  // Create a simple gradient pattern that changes over time
  const int tex_width = 256;
  const int tex_height = 256;
  uint8_t* pixels = (uint8_t*)g_malloc(tex_width * tex_height * 4);

  for (int y = 0; y < tex_height; y++) {
    for (int x = 0; x < tex_width; x++) {
      int idx = (y * tex_width + x) * 4;
      pixels[idx + 0] = (uint8_t)(x + self->color_offset);      // R
      pixels[idx + 1] = (uint8_t)(y + self->color_offset);      // G
      pixels[idx + 2] = (uint8_t)(128 + self->color_offset);    // B
      pixels[idx + 3] = 255;                                     // A
    }
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  g_free(pixels);

  // Increment color offset for animation
  self->color_offset += 2;

  *target = GL_TEXTURE_2D;
  *name = self->texture_id;
  *width = tex_width;
  *height = tex_height;

  // NOTE: We do NOT restore the previous texture binding here.
  // This is intentional to reproduce the bug where GL state is modified
  // without being restored, causing Skia rendering artifacts.

  return TRUE;
}

static void simple_gl_texture_dispose(GObject* object) {
  SimpleGLTexture* self = SIMPLE_GL_TEXTURE(object);

  if (self->initialized && self->texture_id != 0) {
    glDeleteTextures(1, &self->texture_id);
    self->texture_id = 0;
    self->initialized = FALSE;
  }

  G_OBJECT_CLASS(simple_gl_texture_parent_class)->dispose(object);
}

static void simple_gl_texture_class_init(SimpleGLTextureClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = simple_gl_texture_dispose;
  FL_TEXTURE_GL_CLASS(klass)->populate = simple_gl_texture_populate;
}

static void simple_gl_texture_init(SimpleGLTexture* self) {
  self->texture_id = 0;
  self->width = 256;
  self->height = 256;
  self->initialized = FALSE;
  self->color_offset = 0;
}

static SimpleGLTexture* simple_gl_texture_new() {
  return SIMPLE_GL_TEXTURE(g_object_new(simple_gl_texture_get_type(), nullptr));
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
  SimpleGLTexture* texture;
  int64_t texture_id;
  guint timer_id;
};

G_DEFINE_TYPE(FlTextureReproPlugin, fl_texture_repro_plugin, g_object_get_type())

// Timer callback to update texture periodically
static gboolean update_texture_callback(gpointer user_data) {
  FlTextureReproPlugin* self = FL_TEXTURE_REPRO_PLUGIN(user_data);

  if (self->texture != nullptr) {
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
    // Create and register texture
    self->texture = simple_gl_texture_new();

    gboolean registered = fl_texture_registrar_register_texture(
        self->texture_registrar, FL_TEXTURE(self->texture));

    if (registered) {
      self->texture_id = fl_texture_get_id(FL_TEXTURE(self->texture));

      // Start timer to update texture at ~30fps
      self->timer_id = g_timeout_add(33, update_texture_callback, self);

      g_autoptr(FlValue) result = fl_value_new_int(self->texture_id);
      response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
    } else {
      response = FL_METHOD_RESPONSE(
          fl_method_error_response_new("TEXTURE_ERROR", "Failed to register texture", nullptr));
    }
  } else if (strcmp(method, "dispose") == 0) {
    if (self->timer_id != 0) {
      g_source_remove(self->timer_id);
      self->timer_id = 0;
    }

    if (self->texture != nullptr) {
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

  // Store texture registrar for later use
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
