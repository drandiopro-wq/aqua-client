Put these three dependencies here before building:

vendor/imgui/          <- from https://github.com/ocornut/imgui (any recent release)
    copy: imgui*.cpp and imgui*.h  (core files)
    copy: backends/imgui_impl_dx11.cpp/.h
          backends/imgui_impl_win32.cpp/.h
          into vendor/imgui/backends/

vendor/nlohmann/json.hpp  <- single header from https://github.com/nlohmann/json
    (releases -> single header: json.hpp)

vendor/miniaudio.h        <- single header from https://github.com/mackron/miniaudio
    (just the one file)

That's it - xmake.lua already points at these paths.
