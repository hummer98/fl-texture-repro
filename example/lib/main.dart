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

            // Texture display
            Container(
              decoration: BoxDecoration(
                border: Border.all(color: Colors.grey),
                borderRadius: BorderRadius.circular(8),
              ),
              margin: const EdgeInsets.symmetric(horizontal: 16),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(8),
                child: SizedBox(
                  width: 320,
                  height: 240,
                  child: _isInitialized && _textureId != null
                      ? Texture(textureId: _textureId!)
                      : const Center(
                          child: CircularProgressIndicator(),
                        ),
                ),
              ),
            ),

            const SizedBox(height: 16),

            // Status text
            Text(
              _isInitialized
                  ? 'Texture ID: $_textureId'
                  : 'Initializing texture...',
              style: Theme.of(context).textTheme.bodyMedium,
            ),

            const SizedBox(height: 24),

            // Buttons near the texture - artifacts typically appear here
            const Text(
              '↓ Artifacts may appear on these buttons ↓',
              style: TextStyle(
                fontWeight: FontWeight.bold,
                color: Colors.red,
              ),
            ),

            const SizedBox(height: 8),

            // Row of buttons
            Wrap(
              spacing: 8,
              runSpacing: 8,
              alignment: WrapAlignment.center,
              children: [
                ElevatedButton(
                  onPressed: () {
                    setState(() => _buttonPressCount++);
                  },
                  child: const Text('Button 1'),
                ),
                ElevatedButton(
                  onPressed: () {
                    setState(() => _buttonPressCount++);
                  },
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.green,
                    foregroundColor: Colors.white,
                  ),
                  child: const Text('Button 2'),
                ),
                ElevatedButton(
                  onPressed: () {
                    setState(() => _buttonPressCount++);
                  },
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.orange,
                    foregroundColor: Colors.white,
                  ),
                  child: const Text('Button 3'),
                ),
              ],
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
