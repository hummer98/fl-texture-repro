# Plan B: Docker + GPUクラウド方式

## 概要

Flutter Linux開発環境をDockerイメージとして提供し、GPUクラウドサービス上で実行してもらう。

## メリット

- **再現性が高い**: 環境が完全に固定される
- **柔軟性**: 様々なGPUクラウドで動作
- **本番環境に近い**: Colab特有の制約がない

## デメリット

- **有料**: クラウドGPUのコストが発生
- **手順が複雑**: Dockerとクラウドの知識が必要

## 推奨GPUクラウドプロバイダ

| プロバイダ | 価格目安 | 特徴 |
|-----------|---------|------|
| [RunPod](https://www.runpod.io/) | A100 ~$1.19/hr | セットアップ簡単、コンテナネイティブ |
| [Vast.ai](https://vast.ai/) | 最安クラス | マーケットプレイス型 |
| [Paperspace](https://www.paperspace.com/) | RTX A4000~ | Gradient無料枠あり |
| [Vultr](https://www.vultr.com/) | A100 $1.10/hr | $250無料クレジット |
| [Thunder Compute](https://www.thundercompute.com/) | A100 $0.66/hr | 最安クラス |

## Dockerfile

```dockerfile
FROM nvidia/opengl:1.2-glvnd-runtime-ubuntu22.04

# 基本パッケージ
RUN apt-get update && apt-get install -y \
    clang \
    cmake \
    ninja-build \
    pkg-config \
    libgtk-3-dev \
    liblzma-dev \
    git \
    curl \
    unzip \
    xvfb \
    x11vnc \
    xfce4 \
    xfce4-terminal \
    && rm -rf /var/lib/apt/lists/*

# Flutter SDK
ENV FLUTTER_HOME=/opt/flutter
ENV PATH="${FLUTTER_HOME}/bin:${PATH}"
RUN git clone https://github.com/flutter/flutter.git -b stable ${FLUTTER_HOME}
RUN flutter precache --linux

# 再現コード
WORKDIR /app
COPY . /app

# VNCサーバー設定
ENV DISPLAY=:1
EXPOSE 5901

# 起動スクリプト
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
```

## entrypoint.sh

```bash
#!/bin/bash

# Xvfbを起動（仮想ディスプレイ）
Xvfb :1 -screen 0 1920x1080x24 &
sleep 2

# Xfceデスクトップを起動
startxfce4 &
sleep 2

# VNCサーバーを起動
x11vnc -display :1 -forever -usepw -create &

echo "VNC server started on port 5901"
echo "Connect with VNC client to reproduce the issue"

# Flutter Linux appを起動
cd /app/example
flutter run -d linux
```

## 使用方法

### 1. イメージのビルドとpush

```bash
docker build -t ghcr.io/[USERNAME]/fl-texture-repro:latest .
docker push ghcr.io/[USERNAME]/fl-texture-repro:latest
```

### 2. RunPodでの実行例

1. RunPodアカウント作成
2. GPU Pod作成（RTX 3090/4090 or A100推奨）
3. Docker imageを指定: `ghcr.io/[USERNAME]/fl-texture-repro:latest`
4. ポート5901を公開
5. VNCクライアントで接続

### 3. ローカルNVIDIA GPU環境での実行

```bash
docker run --gpus all -p 5901:5901 ghcr.io/[USERNAME]/fl-texture-repro:latest
```

## ディレクトリ構成

```
fl-texture-repro/
├── Dockerfile
├── entrypoint.sh
├── README.md
├── lib/
│   └── main.dart
├── linux/
│   ├── CMakeLists.txt
│   └── texture_plugin.cc
└── example/
    └── ...
```

## 参考リンク

- [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/overview.html)
- [RunPod Documentation](https://docs.runpod.io/)
- [Docker Hub - nvidia/opengl](https://hub.docker.com/r/nvidia/opengl)
