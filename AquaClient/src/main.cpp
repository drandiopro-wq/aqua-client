#include <amethyst/runtime/AmethystContext.hpp>
#include <amethyst/runtime/ModContext.hpp>
#include <amethyst/runtime/events/GameEvents.hpp>
#include "overlay.hpp"
#include "hive.hpp"
#include "replay.hpp"
#include "voice.hpp"

// Game classes ship with the Amethyst 1.21.0.3 headers.
#include <mc/src-client/common/client/ClientInstance.hpp>
#include <mc/src-client/common/client/options/Options.hpp>
#include <mc/src-client/common/client/player/LocalPlayer.hpp>

namespace {
    ClientInstance* g_client = nullptr;
    float g_normalFov = 70.0f;
}

// Settings (also driven live from the ImGui menu)
bool  g_zoomEnabled = true;
bool  g_zooming     = false;   // toggled by L
float g_zoomFov     = 30.0f;
bool  g_autoSprint  = true;
bool  g_statsHud    = true;

ModFunction void Initialize(AmethystContext& ctx, const Amethyst::Mod& mod)
{
    Amethyst::InitializeAmethystMod(ctx, mod);
    Amethyst::EventBus& events = *ctx.mEventBus;

    events.AddListener<OnStartJoinGameEvent>([](const OnStartJoinGameEvent& e) {
        g_client = &e.client;
        if (auto* opts = g_client->getOptions()) {
            g_normalFov = opts->getFloatOption(Options::FloatOption::FOV);
        }
    });

    events.AddListener<OnRequestLeaveGameEvent>([](const OnRequestLeaveGameEvent&) {
        g_client = nullptr;
    });

    // Runs every frame on the client thread
    events.AddListener<UpdateEvent>([](const UpdateEvent&) {
        if (!g_client) return;
        auto* opts = g_client->getOptions();
        if (!opts) return;

        // ---- ZOOM: L toggles (handled in overlay.cpp WndProc) ----
        if (g_zoomEnabled && g_zooming) {
            opts->setFloatOption(Options::FloatOption::FOV, g_zoomFov);
        } else {
            opts->setFloatOption(Options::FloatOption::FOV, g_normalFov);
        }
    });

    // Runs every game tick (20/s)
    events.AddListener<AfterTickEvent>([](const AfterTickEvent& e) {
        if (g_autoSprint && g_client) {
            LocalPlayer* player = g_client->getLocalPlayer();
            if (player && player->isMoving() && !player->isSprinting()) {
                player->setSprinting(true);
            }
        }
        Aqua::Replay::CaptureTick(e.mLevel);
        if (g_client) {
            if (auto* p = g_client->getLocalPlayer()) {
                auto pos = p->getPosition();
                Aqua::Voice::SetLocalPos(pos.x, pos.y, pos.z);
            }
        }
    });

    Aqua::Overlay::Install();   // DX11 hook + aqua ImGui
    Aqua::Hive::Init();         // stats cache/thread
    Aqua::Voice::Init();        // mic + playback
}
