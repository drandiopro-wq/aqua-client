#include "voice.hpp"
#include <imgui.h>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>          // single header -> vendor/miniaudio.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <cstring>
#include <algorithm>

namespace Aqua::Voice {
    bool Enabled = false;
    float Range = 32.0f;
    static ma_device s_cap, s_play;
    static SOCKET s_sock = INVALID_SOCKET;
    static sockaddr_in s_relay{};
    static std::mutex s_mtx;
    static std::queue<std::vector<float>> s_incoming;
    static float s_lx, s_ly, s_lz;
    static bool s_muted = false, s_pttOpenMic = true;
    static char s_room[32] = "aqua";

    static void Send(const void* data, size_t len) {
        if (s_sock == INVALID_SOCKET) return;
        // room-prefixed packet: [len][room][payload]
        std::vector<char> pkt;
        pkt.push_back((char)strlen(s_room));
        pkt.insert(pkt.end(), s_room, s_room + strlen(s_room));
        pkt.insert(pkt.end(), (const char*)data, (const char*)data + len);
        sendto(s_sock, pkt.data(), (int)pkt.size(), 0, (sockaddr*)&s_relay, sizeof(s_relay));
    }

    static void CapCallback(ma_device*, void* out, const void* in, ma_uint32 frames) {
        // hold V = push-to-talk when open mic is off
        bool talk = s_pttOpenMic || (GetAsyncKeyState('V') & 0x8000);
        if (!s_muted && talk) Send(in, frames * 2);   // s16 mono
        (void)out;
    }
    static void PlayCallback(ma_device*, void* out, const void*, ma_uint32 frames) {
        std::lock_guard<std::mutex> l(s_mtx);
        float* o = (float*)out;
        memset(o, 0, frames * 4);
        if (!s_incoming.empty()) {
            auto& buf = s_incoming.front();
            ma_uint32 n = (ma_uint32)std::min<size_t>(frames, buf.size());
            for (ma_uint32 i = 0; i < n; i++) o[i] += buf[i];
            buf.erase(buf.begin(), buf.begin() + n);
            if (buf.empty()) s_incoming.pop();
        }
    }

    void SetLocalPos(float x, float y, float z) { s_lx = x; s_ly = y; s_lz = z; }

    void Init() {
        WSADATA w; WSAStartup(MAKEWORD(2, 2), &w);
        s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        s_relay.sin_family = AF_INET;
        s_relay.sin_port = htons(43770);
        inet_pton(AF_INET, "YOUR.SERVER.IP", &s_relay.sin_addr); // <- EDIT: your relay IP

        ma_device_config c = ma_device_config_capture(1, ma_format_s16, 16000, 480, CapCallback);
        ma_device_init(nullptr, &c, &s_cap);
        ma_device_config p = ma_device_config_playback(1, ma_format_f32, 16000, 480, PlayCallback);
        ma_device_init(nullptr, &p, &s_play);
        ma_device_start(&s_cap); ma_device_start(&s_play);
        Enabled = true;
    }

    void Shutdown() {
        ma_device_uninit(&s_cap); ma_device_uninit(&s_play);
        if (s_sock != INVALID_SOCKET) closesocket(s_sock);
        WSACleanup();
    }

    void ShowWindow() {
        if (!Enabled) return;
        ImGui::Begin("Voice Chat");
        ImGui::InputText("Room", s_room, sizeof(s_room));
        ImGui::Checkbox("Mute", &s_muted);
        ImGui::SliderFloat("Range", &Range, 8.0f, 64.0f, "%.0f blocks");
        ImGui::Text(s_pttOpenMic ? "Open mic" : "Push-to-talk: HOLD V");
        if (ImGui::Button("Toggle PTT")) s_pttOpenMic = !s_pttOpenMic;
        ImGui::End();
    }
}
