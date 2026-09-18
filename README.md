# ESDroid

Android port of [engine_sim](https://github.com/ange-yaghi/engine_sim), Ange
Yaghi's real-time internal combustion engine simulator.

uh it runs natively and uh it uses ndk and it has cool buttons ui controlling i dont know anymore

## Building

Requirements:

- Android SDK (platform 34)
- NDK 26.3.11579264
- CMake 3.22.1
- JDK 17

Command line:

    ./gradlew assembleRelease

or open the project in Android Studio. also release APKs come out unsigned, sign
them with your own key before installing over a previous build.

## Controls

- Left column: STARTER, IGNITION, THROTTLE, CLUTCH, FN, time scale (1x, 1/10x, 1/100x)
- Right column: SHIFT +, SHIFT -, PAUSE, RELOAD, IMPORT, EXIT, CAMERA, OSC PAGE
- CAMERA cycles the view screens, OSC PAGE pages the oscilloscope focus,
  RELOAD re-runs the engine script, IMPORT loads a .mr file from storage
  FN changes all of the other buttons to be different, it works like the function key thats mostly on laptops, im too lazy to say what the other buttons are.

very simple controls

## License

Licensed under the Apache License, Version 2.0 (see LICENSE).

The engine_sim libraries vendored under app/src/main/cpp (engine-sim,
delta-studio, piranha, scs, csv-io) by Ange Yaghi remain under their original
MIT licenses. See NOTICE and the LICENSE file in each of those directories.
Changes made to those libraries for Android support fall under the Apache
2.0 license of the port.


b
