# Plan C: コードレビューベース方式

## 概要

実機環境なしでも進められる方法。最小再現コード + 動画 + 修正PRを提出し、コードレビューベースで修正を受け入れてもらう。

## メリット

- **環境不要**: 開発者が特別な環境を用意する必要なし
- **最速**: 環境構築の手間なく議論を開始できる
- **修正が明確**: 数行の変更で意図が明確

## デメリット

- 開発者が実機確認を求める可能性がある
- マージまでに時間がかかる可能性

## 提出物

### 1. 最小再現コード

```
fl-texture-repro/
├── lib/main.dart           # Texture widget + Button
├── linux/
│   ├── CMakeLists.txt
│   └── texture_plugin.cc   # FlTextureGL実装
├── pubspec.yaml
└── README.md
```

**main.dart** (最小構成):
```dart
import 'package:flutter/material.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        body: Column(
          children: [
            // 外部テクスチャ表示
            SizedBox(
              width: 320,
              height: 240,
              child: Texture(textureId: 1), // プラグインから取得
            ),
            // この付近にアーティファクトが発生
            ElevatedButton(
              onPressed: () {},
              child: const Text('Button'),
            ),
          ],
        ),
      ),
    );
  }
}
```

### 2. 再現動画/スクリーンショット

- **Before**: アーティファクトが発生している状態
- **After**: 修正後の正常な状態
- GIF or MP4形式で提供

### 3. 修正PR

**対象ファイル**: `engine/src/flutter/shell/platform/linux/fl_engine.cc`

**修正内容**:
```c
static gboolean fl_engine_gl_external_texture_frame_callback(
    FlEngine* self,
    FlTextureGL* texture,
    uint32_t width,
    uint32_t height,
    FlutterOpenGLTexture* opengl_texture) {

  // === 追加: 現在のテクスチャバインディングを保存 ===
  GLint previous_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

  // 既存のテクスチャ取得処理
  uint32_t texture_id;
  if (!fl_texture_gl_populate(texture, width, height, &texture_id, nullptr)) {
    return FALSE;
  }

  opengl_texture->target = GL_TEXTURE_2D;
  opengl_texture->name = texture_id;
  opengl_texture->format = GL_RGBA8;

  // === 追加: テクスチャバインディングを復元 ===
  glBindTexture(GL_TEXTURE_2D, previous_texture);

  return TRUE;
}
```

## Issue/PRテンプレート

### Issue更新コメント

```markdown
## Minimal Reproduction

I've created a minimal reproduction repository:
https://github.com/[USERNAME]/fl-texture-repro

### Video demonstrating the issue

[アーティファクトが発生している動画を埋め込み]

### Proposed Fix

The fix is straightforward - save and restore `GL_TEXTURE_BINDING_2D` state:

[コード差分を記載]

### Reproduction Environment Options

If you need to reproduce this on actual hardware:

1. **Google Colab** (Free): [Open in Colab badge + link]
2. **Docker** (Requires GPU cloud): [Docker image link]

The fix can also be verified through code review as the change is minimal and the intent is clear.
```

### PR説明文

```markdown
## Description

Fixes #179322

This PR fixes rendering artifacts that appear on UI elements when using `FlTextureGL` on Linux with NVIDIA GPUs.

## Root Cause

The `fl_engine_gl_external_texture_frame_callback` function modifies `GL_TEXTURE_BINDING_2D` state without saving/restoring it. This causes Skia to render with an unexpected texture bound, resulting in visual artifacts.

## Solution

Save the current `GL_TEXTURE_BINDING_2D` value before texture operations and restore it afterwards.

## Testing

- Tested on Jetson Nano (ARM64 + NVIDIA GPU + Linux)
- Video demonstration: [link]
- Minimal reproduction: [link]

## Checklist

- [ ] Tests pass locally
- [ ] Code follows project style guidelines
```

## 進め方

1. **Step 1**: 最小再現コードを作成してリポジトリ公開
2. **Step 2**: 再現動画を撮影
3. **Step 3**: Issueにコメントで再現情報を追加
4. **Step 4**: Flutter Engineリポジトリに修正PRを提出
5. **Step 5**: 開発者が環境確認を希望した場合、Plan A/Bを提供

## 参考

- [Flutter Engine Contributing Guide](https://github.com/flutter/engine/blob/main/CONTRIBUTING.md)
- [Flutter Issue #179322](https://github.com/flutter/flutter/issues/179322)
