#include "hive.hpp"
#include <windows.h>
#include <winhttp.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <string>
#include <vector>

namespace Aqua::Hive {
    static std::mutex s_mtx;
    static std::string s_status = "No data yet.";
    static std::vector<std::string> s_lines;

    void Init() {}

    static std::wstring ToWide(const std::string& s) {
        return std::wstring(s.begin(), s.end());
    }

    static std::string HttpGet(const std::wstring& urlPath) {
        HINTERNET session = WinHttpOpen(L"AquaClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        nullptr, nullptr, 0);
        if (!session) return {};
        HINTERNET conn = WinHttpConnect(session, L"api.playhive.com",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
        HINTERNET req = conn ? WinHttpOpenRequest(conn, L"GET", urlPath.c_str(),
                     nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE) : nullptr;
        std::string out;
        if (req && WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                     nullptr, 0, 0, 0) && WinHttpReceiveResponse(req, nullptr)) {
            DWORD size = 0;
            do {
                if (!WinHttpQueryDataAvailable(req, &size) || size == 0) break;
                std::string buf(size, '\0');
                DWORD read = 0;
                WinHttpReadData(req, buf.data(), size, &read);
                out.append(buf.data(), read);
            } while (size > 0);
        }
        if (req) WinHttpCloseHandle(req);
        if (conn) WinHttpCloseHandle(conn);
        WinHttpCloseHandle(session);
        return out;
    }

    void RequestStats(const std::string& game, const std::string& player) {
        std::string g = game, p = player;
        std::thread([g, p] {
            { std::lock_guard<std::mutex> l(s_mtx); s_status = "Fetching " + p + "..."; s_lines.clear(); }

            std::string body = HttpGet(L"/api/v0/game/" + ToWide(g) + L"/player/" + ToWide(p));
            // Endpoint path varies slightly by API version - check the Swagger UI:
            // https://api.playhive.com/api/swagger-ui/index.html

            std::lock_guard<std::mutex> l(s_mtx);
            if (body.empty()) { s_status = "Network error / player not found."; return; }
            try {
                auto j = nlohmann::json::parse(body);
                s_lines = { "Player: " + p + "  [" + g + "]" };
                auto add = [&](const char* key, const char* label) {
                    if (j.contains(key) && j[key].is_number())
                        s_lines.push_back(std::string(label) + ": " + j[key].dump());
                };
                add("victories", "Wins"); add("games_played", "Games");
                add("kills", "Kills");     add("deaths", "Deaths");
                add("xp", "XP");           add("level", "Level");
                if (j.contains("kills") && j.contains("deaths")
                    && j["deaths"].get<int>() > 0) {
                    float kdr = (float)j["kills"].get<int>() / j["deaths"].get<int>();
                    s_lines.push_back("KDR: " + std::to_string(kdr).substr(0, 4));
                }
                s_status = "OK";
            } catch (...) { s_status = "Failed to parse API response."; }
        }).detach();
    }

    std::vector<std::string> GetDisplayLines() {
        std::lock_guard<std::mutex> l(s_mtx);
        return s_lines.empty() ? std::vector<std::string>{s_status} : s_lines;
    }
}
