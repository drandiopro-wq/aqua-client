#include "replay.hpp"
#include <imgui.h>
#include <cstdio>
#include <algorithm>
#include <mc/src/common/world/level/Level.hpp>
#include <mc/src/common/world/actor/Actor.hpp>

namespace Aqua::Replay {
    bool Recording = false;
    bool ViewerOpen = false;
    static std::vector<TickSnap> s_session;
    static uint32_t s_tick = 0;
    static int s_viewTick = 0;

    void StartRecording(const std::string&) { s_session.clear(); s_tick = 0; Recording = true; }
    void StopAndSave() {
        Recording = false;
        FILE* f = nullptr;
        if (fopen_s(&f, "aquaclient_replay.bin", "wb") == 0 && f) {
            uint32_t n = (uint32_t)s_session.size();
            fwrite(&n, 4, 1, f);
            for (auto& t : s_session) {
                fwrite(&t.tick, 4, 1, f);
                uint32_t c = (uint32_t)t.entities.size();
                fwrite(&c, 4, 1, f);
                fwrite(t.entities.data(), sizeof(EntitySnap), c, f);
            }
            fclose(f);
        }
    }

    void CaptureTick(Level& level) {
        if (!Recording) return;
        TickSnap snap; snap.tick = s_tick++;
        // Integration point - exact iterator name depends on your Level.hpp:
        for (auto& ent : level.getEntities()) {        // <- verify against headers
            EntitySnap s{};
            s.id  = ent.getActorIdentifier();          // <- verify against headers
            auto pos = ent.getPosition();
            s.x = pos.x; s.y = pos.y; s.z = pos.z;
            s.yaw = ent.getYRot(); s.pitch = ent.getXRot();
            snap.entities.push_back(s);
        }
        s_session.push_back(std::move(snap));
    }

    void ShowViewer() {
        if (!ViewerOpen || s_session.empty()) return;
        ImGui::Begin("Replay Viewer", &ViewerOpen);
        ImGui::SliderInt("Tick", &s_viewTick, 0, (int)s_session.size() - 1);
        if (ImGui::GetIO().KeyShift) s_viewTick++;     // hold Shift to play
        const TickSnap& snap = s_session[s_viewTick];

        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 size(400, 400);
        ImGui::Dummy(size);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                    IM_COL32(0, 200, 200, 255));

        float cx = snap.entities.empty() ? 0 : snap.entities[0].x;
        float cz = snap.entities.empty() ? 0 : snap.entities[0].z;
        for (auto& e : snap.entities) {
            float px = origin.x + size.x / 2 + (e.x - cx) * 4.0f;
            float pz = origin.y + size.y / 2 + (e.z - cz) * 4.0f;
            dl->AddCircleFilled(ImVec2(px, pz), 3.0f, IM_COL32(0, 255, 255, 220));
        }
        ImGui::Text("%zu entities | tick %d", snap.entities.size(), snap.tick);
        ImGui::End();
    }
}
