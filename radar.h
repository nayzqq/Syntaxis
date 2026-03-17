#pragma once

namespace Radar {
    inline bool  enabled = false;
    inline float size = 200.0f;  // размер окна радара в пикселях
    inline float range = 1500.0f; // радиус отображения (юниты CS2)
    inline float posX = 10.0f;   // позиция на экране X
    inline float posY = 10.0f;   // позиция на экране Y
    inline bool  showNames = false;   // подписи имён на радаре
}

void DrawRadar();