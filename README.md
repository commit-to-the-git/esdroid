# ESDroid

Android port of [engine_sim](https://github.com/ange-yaghi/engine_sim), Ange
Yaghi's real-time internal combustion engine simulator. Ported to Android
by F² Cyanic.

The original C++ codebase runs natively through the NDK with an Android
backend: OpenGL ES 3 rendering, OpenSL ES audio, and a touch button overlay in
place of the keyboard. The .mr scripting system is intact, so community
engine files can be imported from device storage at runtime and run as is.

## Building

Requirements:

- Android SDK (platform 34)
- NDK 26.3.11579264
- CMake 3.22.1
- JDK 17

Command line:

    ./gradlew assembleRelease

or open the project in Android Studio. The native code always builds with
optimizations (-O3, LTO, fast math); release APKs come out unsigned, sign
them with your own key before installing over a previous build.

## Controls

Landscape orientation, touch buttons on both screen edges:

- Left column: STARTER, IGNITION, THROTTLE, CLUTCH, time scale (1x, 1/10x, 1/100x)
- Right column: SHIFT +, SHIFT -, PAUSE, RELOAD, IMPORT, EXIT, CAMERA, OSC PAGE
- CAMERA cycles the view screens, OSC PAGE pages the oscilloscope focus,
  RELOAD re-runs the engine script, IMPORT loads a .mr file from storage

## License

Copyright 2026 F² Cyanic

Licensed under the Apache License, Version 2.0 (see LICENSE).

The engine_sim libraries vendored under app/src/main/cpp (engine-sim,
delta-studio, piranha, scs, csv-io) by Ange Yaghi remain under their original
MIT licenses. See NOTICE and the LICENSE file in each of those directories.
Changes made to those libraries for Android support fall under the Apache
2.0 license of the port.
