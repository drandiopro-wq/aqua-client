#pragma once
#include <string>
#include <vector>

namespace Aqua::Hive {
    void Init();
    // game: wars, bed, sky, sg, dr, hide, murder, tw, ctf, build...
    void RequestStats(const std::string& game, const std::string& player);
    // Returns cached, human-readable lines ("Wins: 152", ...). Empty = not loaded yet.
    std::vector<std::string> GetDisplayLines();
}
