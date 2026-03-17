#pragma once
#include "/test/Vector.h"

namespace MyNamespace {
    inline bool menuVisible = true;
    namespace visuals {
        inline bool box = false;
        inline bool skeleton = false;
        inline bool weapon = false;
        inline bool name = false;
        inline bool healthbar = false;
        inline bool reload = false;
        inline bool chams = false;
        inline bool teamchams = false;
        inline bool Aimbot = false;
        inline bool viewDirection = false; // новая опция
    }
    inline bool unhook = false;
}

void draw_Menu();