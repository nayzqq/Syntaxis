#pragma once

namespace RCS {
    inline bool  enabled = false;
    inline float scaleX = 1.0f;   // компенсация по pitch (0.0 - 2.0)
    inline float scaleY = 1.0f;   // компенсация по yaw   (0.0 - 2.0)
}

void RunRCS();