#include "SpectatorList.h"
#include "/test/GUI.h"
#include "/test/ESP.h"
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/Vector.h"
#include "/test/imgui.h"
#include <Windows.h>
#include <vector>
#include <string>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

// m_iObserverMode: 0=none, 1=deathcam, 2=freezecam, 3=fixed, 4=in-eye, 5=chase, 6=roaming
static constexpr std::ptrdiff_t m_iObserverMode  = 0x48; // C_BasePlayerPawn::m_iObserverMode
static constexpr std::ptrdiff_t m_hObserverTarget = 0x4C; // C_BasePlayerPawn::m_hObserverTarget
static constexpr std::ptrdiff_t m_hObserverPawn   = 0x910; // CCSPlayerController::m_hObserverPawn

void DrawSpectatorList()
{
    if (!SpectatorList::enabled) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    uintptr_t localController = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwLocalPlayerController);
    if (!localController) return;

    // Собираем список кто наблюдает за localPawn
    struct SpectatorEntry {
        std::string name;
        int         mode; // 4=глаза, 5=от третьего лица, 6=free
    };
    std::vector<SpectatorEntry> spectators;

    for (int i = 0; i < 64; i++)
    {
        uintptr_t controller = GetBaseEntity(i, client);
        if (!controller || controller == localController) continue;

        const int hp = *reinterpret_cast<int*>(
            controller + CBasePlayerController::m_hPawn); // используем как проверку существования
        
        // Получаем observer pawn через controller
        uint32_t hObsPawn = *reinterpret_cast<uint32_t*>(controller + m_hObserverPawn);
        if (!hObsPawn || hObsPawn == 0xFFFFFFFF) continue;

        uintptr_t obsPawn = GetBaseEntityFromHandle(hObsPawn, client);
        if (!obsPawn) continue;

        // Кого наблюдает
        uint32_t hTarget = *reinterpret_cast<uint32_t*>(obsPawn + m_hObserverTarget);
        if (!hTarget || hTarget == 0xFFFFFFFF) continue;

        uintptr_t targetPawn = GetBaseEntityFromHandle(hTarget, client);
        if (targetPawn != localPawn) continue; // не за нами

        const int mode = *reinterpret_cast<int*>(obsPawn + m_iObserverMode);
        if (mode == 0) continue; // не наблюдает

        // Имя
        const char* raw = reinterpret_cast<const char*>(
            controller + CBasePlayerController::m_iszPlayerName);
        std::string name = (raw && raw[0]) ? raw : "unknown";

        spectators.push_back({ name, mode });
    }

    if (spectators.empty()) return;

    // ── Рендер ────────────────────────────────────────────────────────────
    ImDrawList* dl   = ImGui::GetForegroundDrawList();
    const float sw   = ImGui::GetIO().DisplaySize.x;
    const float pad  = 8.0f;
    const float lineH = 16.0f;
    const float w    = 160.0f;
    const float h    = pad * 2 + lineH * (1 + static_cast<float>(spectators.size()));

    // Позиция: правый верхний угол
    const float x = sw - w - 10.0f;
    const float y = 10.0f;

    // Фон
    dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
        IM_COL32(0, 0, 0, 160), 4.0f);
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
        IM_COL32(200, 50, 50, 200), 4.0f, 0, 1.0f);

    // Заголовок
    dl->AddText(ImVec2(x + pad, y + pad),
        IM_COL32(255, 80, 80, 255), "SPECTATORS");

    // Список
    for (int i = 0; i < (int)spectators.size(); i++)
    {
        const auto& s   = spectators[i];
        const float ly  = y + pad + lineH * (i + 1);

        // Режим
        const char* modeStr = "?";
        switch (s.mode) {
        case 4: modeStr = "[EYE]";   break;
        case 5: modeStr = "[3RD]";   break;
        case 6: modeStr = "[FREE]";  break;
        }

        char buf[160];
        snprintf(buf, sizeof(buf), "%s %s", modeStr, s.name.c_str());
        dl->AddText(ImVec2(x + pad, ly), IM_COL32(255, 220, 220, 255), buf);
    }
}
