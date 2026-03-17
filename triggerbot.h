#pragma once

namespace TriggerBot {
    inline bool  enabled = false;
    inline int   hotkey = 0;      // 0=всегда когда включЄн, 1=ALT, 2=SHIFT, 3=X1
    inline int   delayMs = 50;     // задержка перед выстрелом (мс) Ч дл€ легита
    inline bool  headshotOnly = false; // стрел€ть только если прицел на голове
}

void RunTriggerBot();