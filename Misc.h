#pragma once
#include <Windows.h>

namespace Misc {
    // ── Panic Key ─────────────────────────────────────────────────────────
    // Одна кнопка мгновенно выключает ВСЕ фичи (не unhook — только скрывает)
    inline bool panicEnabled = true;
    inline int  panicKey     = VK_F12;  // Default: F12

    // ── Watermark ─────────────────────────────────────────────────────────
    inline bool watermark    = true;    // FPS + время в углу

    // ── Corner Box ────────────────────────────────────────────────────────
    // true  = угловой стиль ╔╗╚╝
    // false = полный прямоугольник (старый стиль)
    inline bool cornerBox    = false;
}

void RunMisc();          // вызывается каждый кадр (panic key check)
void DrawWatermark();    // вызывается в render loop
