#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Level;

namespace Aqua::Replay {
    struct EntitySnap { uint64_t id; uint8_t type; float x, y, z, yaw, pitch; };
    struct TickSnap  { uint32_t tick; std::vector<EntitySnap> entities; };

    extern bool Recording;
    extern bool ViewerOpen;
    void StartRecording(const std::string& path);
    void StopAndSave();
    void CaptureTick(Level& level);  // call from AfterTickEvent
    void ShowViewer();               // ImGui window, call every frame from overlay
}
