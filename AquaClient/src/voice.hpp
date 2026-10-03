#pragma once
namespace Aqua::Voice {
    extern bool Enabled;
    extern float Range;      // blocks
    void Init();             // start capture+playback threads
    void Shutdown();
    void SetLocalPos(float x, float y, float z);   // call every tick
    void ShowWindow();       // ImGui: room, mute, volume, PTT status
}
