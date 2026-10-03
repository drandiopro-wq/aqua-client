# AquaClient

A client-side mod for Minecraft Bedrock (Windows), built on Amethyst.
Zoom (toggle with **L**), Auto Sprint, Hive Statistics, Replay recorder/viewer, Voice Chat, aqua-themed ImGui menu.

## Setup
1. Install the Amethyst Launcher: https://github.com/FrederoxDev/Amethyst-Launcher
2. Clone Amethyst (1.21.0.3 branch):
   git clone -b 1.21.0.3 https://github.com/FrederoxDev/Amethyst
3. Set the environment variable AMETHYST_SRC to that folder.
4. Install xmake (https://xmake.io) + Visual Studio 2022 (C++ workload).
5. Fill the vendor folder (see vendor/README.md).
6. Run: xmake
   -> builds AquaClient.dll and copies it to your amethyst mods folder.

## Or let GitHub build it
Push this folder to a GitHub repo - the included workflow builds the DLL
and uploads it as a downloadable artifact (see .github/workflows/build.yml).

## Voice chat
Host tools/voice_relay.py anywhere (python voice_relay.py), put its IP in
src/voice.cpp (inet_pton line). Everyone in the same room name can hear each other.

## Notes
- src/replay.cpp and src/main.cpp contain two lines marked
  "verify against Level.hpp" - entity iterator/method names depend on your header set.
- src/overlay.cpp: the DX11 Present hook needs MinHook wiring (see note at bottom).
