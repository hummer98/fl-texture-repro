import 'package:flutter/material.dart';
import 'package:fl_texture_repro/fl_texture_repro.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'FlTextureGL Artifact Reproduction',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
        useMaterial3: true,
      ),
      home: const TextureReproPage(),
    );
  }
}

class TextureReproPage extends StatefulWidget {
  const TextureReproPage({super.key});

  @override
  State<TextureReproPage> createState() => _TextureReproPageState();
}

class _TextureReproPageState extends State<TextureReproPage> {
  int? _textureId;
  bool _isInitialized = false;
  bool _cameraEnabled = true;
  int _buttonPressCount = 0;

  @override
  void initState() {
    super.initState();
    _initializeTexture();
  }

  Future<void> _initializeTexture() async {
    try {
      final textureId = await FlTextureRepro.initialize();
      if (mounted) {
        setState(() {
          _textureId = textureId;
          _isInitialized = true;
        });
      }
    } catch (e) {
      debugPrint('Failed to initialize texture: $e');
    }
  }

  void _toggleCamera() {
    // Just toggle visibility - don't restart pipeline to avoid crash
    setState(() {
      _cameraEnabled = !_cameraEnabled;
    });
  }

  @override
  void dispose() {
    FlTextureRepro.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('FlTextureGL Artifact Repro'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
      ),
      body: SingleChildScrollView(
        child: Column(
          children: [
            const SizedBox(height: 16),

            // Description
            const Padding(
              padding: EdgeInsets.symmetric(horizontal: 16),
              child: Text(
                'This app demonstrates rendering artifacts that occur when '
                'using FlTextureGL on Linux with NVIDIA GPUs. '
                'The artifacts typically appear as black shadows or glitches '
                'on the UI elements (buttons, text) near the texture.',
                textAlign: TextAlign.center,
              ),
            ),

            const SizedBox(height: 24),

            // Texture with various UI elements on all 4 sides
            Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                // Top: Text + Icon + Button
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Icon(Icons.arrow_upward, color: Colors.red),
                    const SizedBox(width: 8),
                    const Text('TOP TEXT', style: TextStyle(fontWeight: FontWeight.bold)),
                    const SizedBox(width: 8),
                    ElevatedButton(
                      onPressed: () => setState(() => _buttonPressCount++),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.red,
                        foregroundColor: Colors.white,
                      ),
                      child: const Text('TOP'),
                    ),
                    const SizedBox(width: 8),
                    const Chip(label: Text('Chip')),
                  ],
                ),
                const SizedBox(height: 4),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    // Left: Various elements stacked
                    Column(
                      children: [
                        ElevatedButton(
                          onPressed: () => setState(() => _buttonPressCount++),
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.blue,
                            foregroundColor: Colors.white,
                          ),
                          child: const Text('LEFT'),
                        ),
                        const SizedBox(height: 4),
                        const Icon(Icons.arrow_back, color: Colors.blue),
                        const SizedBox(height: 4),
                        const Text('L', style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold)),
                        const SizedBox(height: 4),
                        Checkbox(value: true, onChanged: (_) {}),
                      ],
                    ),
                    const SizedBox(width: 4),
                    // Texture display
                    Container(
                      decoration: BoxDecoration(
                        border: Border.all(color: Colors.grey, width: 2),
                      ),
                      child: SizedBox(
                        width: 320,
                        height: 240,
                        child: _cameraEnabled && _isInitialized && _textureId != null
                            ? Texture(textureId: _textureId!)
                            : Center(
                                child: _cameraEnabled
                                    ? const CircularProgressIndicator()
                                    : const Text('Camera OFF', style: TextStyle(color: Colors.grey)),
                              ),
                      ),
                    ),
                    const SizedBox(width: 4),
                    // Right: Various elements stacked
                    Column(
                      children: [
                        ElevatedButton(
                          onPressed: () => setState(() => _buttonPressCount++),
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.green,
                            foregroundColor: Colors.white,
                          ),
                          child: const Text('RIGHT'),
                        ),
                        const SizedBox(height: 4),
                        const Icon(Icons.arrow_forward, color: Colors.green),
                        const SizedBox(height: 4),
                        const Text('R', style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold)),
                        const SizedBox(height: 4),
                        Switch(value: true, onChanged: (_) {}),
                      ],
                    ),
                  ],
                ),
                const SizedBox(height: 4),
                // Bottom: Text + Icon + Button
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Icon(Icons.arrow_downward, color: Colors.orange),
                    const SizedBox(width: 8),
                    const Text('BOTTOM TEXT', style: TextStyle(fontWeight: FontWeight.bold)),
                    const SizedBox(width: 8),
                    ElevatedButton(
                      onPressed: () => setState(() => _buttonPressCount++),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.orange,
                        foregroundColor: Colors.white,
                      ),
                      child: const Text('BOTTOM'),
                    ),
                    const SizedBox(width: 8),
                    IconButton(
                      icon: const Icon(Icons.star, color: Colors.amber),
                      onPressed: () {},
                    ),
                  ],
                ),
              ],
            ),

            const SizedBox(height: 16),

            // Camera On/Off switch
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Text('Camera: ', style: TextStyle(fontWeight: FontWeight.bold)),
                Switch(
                  value: _cameraEnabled,
                  onChanged: (_) => _toggleCamera(),
                  activeColor: Colors.green,
                ),
                Text(_cameraEnabled ? 'ON' : 'OFF'),
              ],
            ),

            const SizedBox(height: 8),

            // Status text
            Text(
              _isInitialized
                  ? 'Texture ID: $_textureId | Clicks: $_buttonPressCount'
                  : _cameraEnabled ? 'Initializing texture...' : 'Camera OFF',
              style: Theme.of(context).textTheme.bodyMedium,
            ),

            const SizedBox(height: 16),

            // More UI elements that may show artifacts
            Card(
              margin: const EdgeInsets.symmetric(horizontal: 16),
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  children: [
                    const Text(
                      'Card with various UI elements',
                      style: TextStyle(fontWeight: FontWeight.bold),
                    ),
                    const SizedBox(height: 8),
                    Text('Button press count: $_buttonPressCount'),
                    const SizedBox(height: 8),
                    const TextField(
                      decoration: InputDecoration(
                        border: OutlineInputBorder(),
                        labelText: 'Text input',
                        hintText: 'Type something...',
                      ),
                    ),
                    const SizedBox(height: 8),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                      children: [
                        IconButton(
                          icon: const Icon(Icons.star),
                          onPressed: () {},
                        ),
                        IconButton(
                          icon: const Icon(Icons.favorite),
                          onPressed: () {},
                        ),
                        IconButton(
                          icon: const Icon(Icons.share),
                          onPressed: () {},
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 24),

            // Instructions
            Container(
              margin: const EdgeInsets.symmetric(horizontal: 16),
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.yellow.shade100,
                borderRadius: BorderRadius.circular(8),
              ),
              child: const Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'How to reproduce:',
                    style: TextStyle(fontWeight: FontWeight.bold),
                  ),
                  SizedBox(height: 8),
                  Text('1. Run this app on Linux with NVIDIA GPU'),
                  Text('2. Observe the texture animation above'),
                  Text('3. Look for black shadows/artifacts on buttons'),
                  Text('4. Click buttons to trigger redraws'),
                  Text('5. Artifacts may appear intermittently'),
                ],
              ),
            ),

            const SizedBox(height: 32),
          ],
        ),
      ),
    );
  }
}
