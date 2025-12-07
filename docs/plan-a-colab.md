# Plan A: Google Colab + VNC 方式

## 概要

Google Colabの無料GPUインスタンスでFlutter Linux環境を構築し、VNC経由でGUIアクセスを提供する。

## メリット

- **無料**: 開発者にコスト負担なし
- **ワンクリック**: ノートブックを開くだけで環境構築
- **GPU対応**: T4 GPU（NVIDIA）が使用可能
- **VirtualGL**: OpenGLアプリのGPUアクセラレーション対応

## デメリット

- セッション時間制限（GPU: 4時間/24時間）
- T4のドライバ挙動がJetsonと同一か不明
- ngrok等の外部サービスに依存

## 必要なツール

- [remocolab](https://github.com/demotomohiro/remocolab) - VNC + SSH セットアップ
- [TurboVNC](https://turbovnc.org/) - VNCクライアント
- [VirtualGL](https://virtualgl.org/) - OpenGL GPU転送

## 実装手順

### 1. Colabノートブック作成

```python
# セル1: 基本セットアップ
!apt-get update
!apt-get install -y clang cmake ninja-build pkg-config libgtk-3-dev liblzma-dev

# Flutter SDKインストール
!git clone https://github.com/flutter/flutter.git -b stable /content/flutter
import os
os.environ['PATH'] = '/content/flutter/bin:' + os.environ['PATH']
!flutter doctor

# セル2: VNC + VirtualGLセットアップ
!pip install git+https://github.com/demotomohiro/remocolab.git
import remocolab
remocolab.setupVNC()

# セル3: 再現コードをclone & 実行
!git clone https://github.com/[YOUR_REPO]/fl-texture-repro.git
%cd fl-texture-repro/example
!flutter run -d linux
```

### 2. 開発者への提供方法

1. GitHubリポジトリに`repro.ipynb`を配置
2. "Open in Colab" バッジを追加:
   ```markdown
   [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/[YOUR_REPO]/blob/main/repro.ipynb)
   ```
3. VNCクライアントの接続手順をREADMEに記載

### 3. GPU確認コマンド

```bash
# GPU認識確認
nvidia-smi

# OpenGLレンダラ確認（VirtualGL経由）
vglrun glxinfo | grep "OpenGL renderer"
```

## 検証が必要な項目

- [ ] Colab T4でFlTextureGLのバグが再現するか
- [ ] VirtualGL経由でOpenGL状態管理の問題が発生するか
- [ ] flutter run -d linuxがColab環境で正常動作するか

## 参考リンク

- [remocolab GitHub](https://github.com/demotomohiro/remocolab)
- [How to run OpenGL desktop programs on Google Colaboratory](https://internet-of-tomohiro.netlify.app/google_colab/vnc.en.html)
- [Google Colab GPU runtime](https://colab.research.google.com/)
