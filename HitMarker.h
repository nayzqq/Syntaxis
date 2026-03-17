#pragma once

namespace HitMarker {
    inline bool  enabled     = false;
    inline float duration    = 0.4f;   // секунд отображения
    inline int   size        = 8;      // длина линий крестика
    inline bool  showDamage  = true;   // показывать цифру урона
}

void DrawHitMarker();
