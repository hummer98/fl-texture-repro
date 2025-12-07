import 'package:flutter/services.dart';

class FlTextureRepro {
  static const MethodChannel _channel = MethodChannel('fl_texture_repro');

  /// Initialize the texture plugin and return the texture ID
  static Future<int> initialize() async {
    final int textureId = await _channel.invokeMethod('initialize');
    return textureId;
  }

  /// Dispose the texture
  static Future<void> dispose() async {
    await _channel.invokeMethod('dispose');
  }
}
