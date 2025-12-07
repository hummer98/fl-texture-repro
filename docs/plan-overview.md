# FlTextureGL Issue 再現環境提供計画

Flutter Issue [#179322](https://github.com/flutter/flutter/issues/179322) の再現環境を提供するための計画案をまとめます。

## 背景

- **問題**: Linux + NVIDIA GPU環境で`FlTextureGL`使用時にUIにレンダリングアーティファクトが発生
- **原因**: OpenGLのテクスチャバインディング状態が保存/復元されていない
- **課題**: Flutter開発者がLinux + NVIDIA GPU環境を持っていない可能性が高い

## 計画案一覧

| 計画 | 概要 | コスト | 開発者の負担 |
|------|------|--------|-------------|
| [Plan A](./plan-a-colab.md) | Google Colab + VNC | 無料 | 低 |
| [Plan B](./plan-b-docker.md) | Docker + GPUクラウド | 有料 | 中 |
| [Plan C](./plan-c-code-review.md) | コードレビューベース | 無料 | 最小 |

## 推奨

**Plan A + Plan C の組み合わせ**を推奨します。

1. まず**Plan C**（コードレビューベース）で修正PRを提出
2. 開発者が実機確認を希望した場合に**Plan A**（Colab）を提供
3. Colabで動作しない場合の保険として**Plan B**（Docker）も用意

## 成果物

- [ ] 最小再現コード（Flutter Linux plugin）
- [ ] 再現動画/スクリーンショット
- [ ] Google Colab ノートブック
- [ ] Dockerfile（GPU対応）
- [ ] 修正PR
