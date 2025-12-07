# FlTextureGL Rendering Artifact Reproduction

This is a minimal reproduction project for [Flutter Issue #179322](https://github.com/flutter/flutter/issues/179322).

## Problem

When using `FlTextureGL` on Linux with NVIDIA GPUs, UI elements (buttons, text, etc.) near the texture display intermittent rendering artifacts that appear as black shadows or glitches.

## Root Cause

The OpenGL texture binding state (`GL_TEXTURE_BINDING_2D`) is modified during texture population but not restored afterwards. This causes Skia to render with an unexpected texture bound, resulting in visual artifacts.

## Reproduction Environment

- **OS**: Linux (tested on Jetson/ARM64 and x86_64)
- **GPU**: NVIDIA (required)
- **Flutter**: Any recent stable version

## Quick Start

### Option 1: Google Colab (Recommended)

[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/YOUR_USERNAME/fl-texture-repro/blob/main/repro.ipynb)

### Option 2: Local Build

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install clang cmake ninja-build pkg-config libgtk-3-dev liblzma-dev libepoxy-dev

# Clone and run
cd fl_texture_repro/example
flutter run -d linux
```

## Expected Behavior

- Texture displays animated gradient
- UI elements render cleanly without artifacts

## Actual Behavior

- Black shadows/glitches appear intermittently on buttons and text
- Artifacts are more visible when texture updates frequently

## Proposed Fix

Save and restore `GL_TEXTURE_BINDING_2D` state in `fl_engine_gl_external_texture_frame_callback`:

```c
GLint previous_texture;
glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

// ... texture operations ...

glBindTexture(GL_TEXTURE_2D, previous_texture);  // Restore
```

## Project Structure

```
fl_texture_repro/
├── lib/
│   └── fl_texture_repro.dart      # Dart API
├── linux/
│   ├── CMakeLists.txt             # Build config with OpenGL
│   └── fl_texture_repro_plugin.cc # FlTextureGL implementation
├── example/
│   └── lib/main.dart              # Demo app with Texture + buttons
└── repro.ipynb                    # Google Colab notebook
```

## Related

- [Flutter Issue #179322](https://github.com/flutter/flutter/issues/179322)
- [FlTextureGL Documentation](https://api.flutter.dev/flutter/flutter_linux/FlTextureGL-class.html)
