#include "BombTimer.h"
#include "/test/GUI.h"
#include "/test/ESP.h"
#include "output/client_dll.hpp"
#include "output/offsets.hpp"
#include "/test/Vector.h"
#include "/test/imgui.h"
#include <Windows.h>
#include <cmath>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

// C_PlantedC4 offsets из client_dll.cs
static constexpr std::ptrdiff_t m_flC4Blow          = 0x11A0; // GameTime_t (float)
static constexpr std::ptrdiff_t m_flTimerLength      = 0x11A8; // float  (обычно 40с)
static constexpr std::ptrdiff_t m_bBeingDefused      = 0x11AC; // bool
static constexpr std::ptrdiff_t m_flDefuseLength     = 0x11BC; // float  (5с без кита / 10с с китом)
static constexpr std::ptrdiff_t m_flDefuseCountDown  = 0x11C0; // GameTime_t
static constexpr std::ptrdiff_t m_bBombDefused       = 0x11C4; // bool

// CCSGameRules::m_flCurrentTime для получения серверного времени
static constexpr std::ptrdiff_t m_flCurrentTime      = 0x660;

// CCSGameRules::m_bFreezePeriod
static constexpr std::ptrdiff_t m_bFreezePeriod      = 0x40;

void DrawBombTimer()
{
    if (!BombTimer::enabled) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    // Получаем PlantedC4
    uintptr_t plantedC4ptr = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwPlantedC4);
    if (!plantedC4ptr) return;

    // dwPlantedC4 — это указатель на список, первый элемент
    uintptr_t plantedC4 = *reinterpret_cast<uintptr_t*>(plantedC4ptr);
    if (!plantedC4) return;

    const bool defused   = *reinterpret_cast<bool*>(plantedC4 + m_bBombDefused);
    if (defused) return; // бомба обезврежена — не показываем

    const float blowTime     = *reinterpret_cast<float*>(plantedC4 + m_flC4Blow);
    const float timerLength  = *reinterpret_cast<float*>(plantedC4 + m_flTimerLength);
    const bool  beingDefused = *reinterpret_cast<bool*>(plantedC4 + m_bBeingDefused);
    const float defuseCD     = *reinterpret_cast<float*>(plantedC4 + m_flDefuseCountDown);
    const float defuseLen    = *reinterpret_cast<float*>(plantedC4 + m_flDefuseLength);

    // Серверное время из GameRules
    uintptr_t gameRules = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwGameRules);
    if (!gameRules) return;

    const float curTime = *reinterpret_cast<float*>(gameRules + m_flCurrentTime);

    const float timeLeft    = blowTime - curTime;
    if (timeLeft <= 0.0f) return;

    const float defuseLeft  = beingDefused ? (defuseCD - curTime) : defuseLen;
    const float progress    = 1.0f - (timeLeft / timerLength); // 0→1

    // ── Рендер ────────────────────────────────────────────────────────────
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float sw = ImGui::GetIO().DisplaySize.x;
    const float sh = ImGui::GetIO().DisplaySize.y;

    const float boxW  = 220.0f;
    const float boxH  = beingDefused ? 72.0f : 52.0f;
    const float bx    = (sw - boxW) * 0.5f;
    const float by    = sh * 0.75f;

    // Фон
    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + boxW, by + boxH),
        IM_COL32(0, 0, 0, 180), 4.0f);

    // Цвет по времени: зелёный > 10с, жёлтый 5-10с, красный < 5с
    ImU32 timerColor;
    if (timeLeft > 10.0f)      timerColor = IM_COL32(50,  255, 50,  255);
    else if (timeLeft > 5.0f)  timerColor = IM_COL32(255, 200, 0,   255);
    else                       timerColor = IM_COL32(255, 50,  50,  255);

    // Граница — мигает при timeLeft < 5
    bool showBorder = (timeLeft > 5.0f) || (static_cast<int>(curTime * 4) % 2 == 0);
    if (showBorder)
        dl->AddRect(ImVec2(bx, by), ImVec2(bx + boxW, by + boxH),
            timerColor, 4.0f, 0, 1.5f);

    // Текст таймера
    char timeBuf[64];
    snprintf(timeBuf, sizeof(timeBuf), "BOMB  %.2fs", timeLeft);
    const ImVec2 tSz = ImGui::CalcTextSize(timeBuf);
    dl->AddText(ImVec2(bx + (boxW - tSz.x) * 0.5f, by + 6.0f),
        timerColor, timeBuf);

    // Прогресс-бар
    const float barX1 = bx + 8.0f;
    const float barX2 = bx + boxW - 8.0f;
    const float barY  = by + 26.0f;
    const float barH  = 6.0f;
    dl->AddRectFilled(ImVec2(barX1, barY), ImVec2(barX2, barY + barH),
        IM_COL32(40, 40, 40, 255), 2.0f);
    dl->AddRectFilled(ImVec2(barX1, barY),
        ImVec2(barX1 + (barX2 - barX1) * progress, barY + barH),
        timerColor, 2.0f);

    // Defuse
    if (beingDefused)
    {
        ImU32 defColor = (defuseLeft < timeLeft)
            ? IM_COL32(50, 255, 50, 255)   // успеет
            : IM_COL32(255, 80, 80, 255);  // не успеет

        char defBuf[64];
        snprintf(defBuf, sizeof(defBuf), "DEFUSE  %.2fs", defuseLeft > 0 ? defuseLeft : 0.0f);
        const ImVec2 dSz = ImGui::CalcTextSize(defBuf);
        dl->AddText(ImVec2(bx + (boxW - dSz.x) * 0.5f, by + 38.0f),
            defColor, defBuf);
    }
}
