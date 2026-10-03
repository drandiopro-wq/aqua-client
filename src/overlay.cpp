#include "overlay.hpp"
#include "hive.hpp"
#include "replay.hpp"
#include "voice.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

// defined in main.cpp
extern bool  g_zoomEnabled, g_zooming, g_autoSprint, g_statsHud;
extern float g_zoomFov;

namespace Aqua::Overlay {

    static IDXGISwapChain* g_swap = nullptr;
    static ID3D11Device*   g_device = nullptr;
    static ID3D11DeviceContext* g_ctx = nullptr;
    static ID3D11RenderTargetView* g_rtv = nullptr;
    static WNDPROC g_origWndProc = nullptr;

    static char s_player[64] = "";
    static int  s_game = 0;
    static const char* s_games[] = { "wars", "bed", "sky", "sg", "dr",
                                     "hide", "murder", "tw", "ctf", "build" };

    // ---------- Aqua theme ----------
    static void ApplyAquaTheme() {
        ImGuiStyle& s = ImGui::GetStyle();
        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]      = ImVec4(0.02f, 0.10f, 0.12f, 0.94f);
        c[ImGuiCol_Border]        = ImVec4(0.00f, 0.90f, 0.90f, 0.65f);
        c[ImGuiCol_FrameBg]       = ImVec4(0.04f, 0.22f, 0.26f, 1.00f);
        c[ImGuiCol_FrameBgHovered]= ImVec4(0.06f, 0.35f, 0.40f, 1.00f);
        c[ImGuiCol_Header]        = ImVec4(0.00f, 0.55f, 0.60f, 1.00f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.75f, 0.80f, 1.00f);
        c[ImGuiCol_HeaderActive]  = ImVec4(0.00f, 0.90f, 0.95f, 1.00f);
        c[ImGuiCol_Button]        = ImVec4(0.00f, 0.45f, 0.50f, 1.00f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 0.70f, 0.75f, 1.00f);
        c[ImGuiCol_ButtonActive]  = ImVec4(0.00f, 0.85f, 0.90f, 1.00f);
        c[ImGuiCol_SliderGrab]    = ImVec4(0.00f, 0.85f, 0.90f, 1.00f);
        c[ImGuiCol_Text]          = ImVec4(0.75f, 1.00f, 1.00f, 1.00f);
        c[ImGuiCol_CheckMark]     = ImVec4(0.20f, 1.00f, 1.00f, 1.00f);
        s.FrameRounding = 6.0f; s.WindowRounding = 8.0f; s.GrabRounding = 6.0f;
    }

    // ---------- WndProc: L toggles zoom ----------
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_KEYDOWN && wParam == 'L' && !(lParam & 0x40000000)) // ignore auto-repeat
            g_zooming = !g_zooming;
        extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
        return CallWindowProc(g_origWndProc, hWnd, msg, wParam, lParam);
    }

    static void RenderFrame() {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ---- Menu ----
        ImGui::Begin("Aqua Client", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(0.2f, 1, 1, 1), "~ aqua ~");
        ImGui::Checkbox("Zoom (toggle with L)", &g_zoomEnabled);
        ImGui::SliderFloat("Zoom FOV", &g_zoomFov, 5.0f, 60.0f, "%.0f");
        ImGui::Checkbox("Auto Sprint", &g_autoSprint);
        ImGui::Checkbox("Hive Stats HUD", &g_statsHud);
        ImGui::Checkbox("Replay Viewer", &Aqua::Replay::ViewerOpen);
        if (ImGui::Button(Aqua::Replay::Recording ? "Stop Recording" : "Start Recording")) {
            if (Aqua::Replay::Recording) Aqua::Replay::StopAndSave();
            else Aqua::Replay::StartRecording("");
        }
        ImGui::End();

        if (g_statsHud) {
            ImGui::Begin("Hive Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Combo("Game", &s_game, s_games, IM_ARRAYSIZE(s_games));
            ImGui::InputTextWithHint("Player", "gamertag", s_player, sizeof(s_player));
            if (ImGui::Button("Fetch", ImVec2(-1, 0)))
                Aqua::Hive::RequestStats(s_games[s_game], s_player);
            ImGui::Separator();
            for (auto& line : Aqua::Hive::GetDisplayLines())
                ImGui::TextUnformatted(line.c_str());
            ImGui::End();
        }

        Aqua::Replay::ShowViewer();
        Aqua::Voice::ShowWindow();

        ImGui::EndFrame();
        ImGui::Render();
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    // ---------- DXGI Present hook ----------
    static HRESULT (STDMETHODCALLTYPE *Present_orig)(IDXGISwapChain*, UINT, UINT);
    static HRESULT STDMETHODCALLTYPE Present_hk(IDXGISwapChain* sc, UINT sync, UINT flags) {
        if (!g_device) {
            g_swap = sc;
            sc->GetDevice(IID_PPV_ARGS(&g_device));
            g_device->GetImmediateContext(&g_ctx);
            ID3D11Texture2D* back = nullptr;
            sc->GetBuffer(0, IID_PPV_ARGS(&back));
            if (back) { g_device->CreateRenderTargetView(back, nullptr, &g_rtv); back->Release(); }

            ImGui::CreateContext();
            ApplyAquaTheme();
            ImGui_ImplWin32_Init(g_swap->GetDesc().OutputWindow);
            ImGui_ImplDX11_Init(g_device, g_ctx);
            g_origWndProc = (WNDPROC)SetWindowLongPtrW(
                g_swap->GetDesc().OutputWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
        }
        RenderFrame();
        return Present_orig(sc, sync, flags);
    }

    void Install() {
        // DX11 hook wiring: hook D3D11CreateDeviceAndSwapChain with MinHook (vendor it),
        // create a dummy swap chain to read the real vtable, then MH_CreateHook on
        // IDXGISwapChain::Present pointing at Present_hk above. Standard pattern used
        // by every Bedrock overlay - wire it here once MinHook is vendored.
        (void)Present_hk; (void)Present_orig;
    }
}
