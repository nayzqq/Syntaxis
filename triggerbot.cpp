#include "TriggerBot.h"
#include "/test/GUI.h"
#include "/test/ESP.h"
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/Vector.h"
#include <Windows.h>
#include <chrono>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

// ИСПРАВЛЕНО: был 0x1854 — неверно. Правильный из client_dll.cs
static constexpr std::ptrdiff_t m_iIDEntIndex = 0x3EAC;

static bool IsTriggerKeyDown()
{
    switch (TriggerBot::hotkey) {
    case 0: return true;
    case 1: return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    case 2: return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    case 3: return (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
    default: return false;
    }
}

// Состояние триггера
static bool s_waiting = false;
static int  s_lastTarget = -1;
static auto s_triggerTime = std::chrono::steady_clock::now();

void RunTriggerBot()
{
    if (!TriggerBot::enabled) return;
    if (!IsTriggerKeyDown()) {
        // Клавиша отпущена — сброс
        s_waiting = false; s_lastTarget = -1;
        return;
    }

    // Не стрелять если ЛКМ уже зажата вручную
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (*reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth) <= 0) return;

    const int localTeam = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iTeamNum);

    // Entity под прицелом
    const int crosshairEntIndex = *reinterpret_cast<int*>(localPawn + m_iIDEntIndex);
    if (crosshairEntIndex <= 0 || crosshairEntIndex > 64) {
        s_waiting = false; s_lastTarget = -1;
        return;
    }

    uintptr_t targetController = GetBaseEntity(crosshairEntIndex - 1, client);
    if (!targetController) return;

    uint32_t hPawn = *reinterpret_cast<uint32_t*>(
        targetController + CBasePlayerController::m_hPawn);
    if (!hPawn || hPawn == 0xFFFFFFFF) return;

    uintptr_t targetPawn = GetBaseEntityFromHandle(hPawn, client);
    if (!targetPawn || targetPawn == localPawn) return;

    const int team = *reinterpret_cast<int*>(targetPawn + C_BaseEntity::m_iTeamNum);
    const int health = *reinterpret_cast<int*>(targetPawn + C_BaseEntity::m_iHealth);
    if (team == localTeam || health <= 0) return;

    // Цель сменилась — перезапускаем таймер
    if (crosshairEntIndex != s_lastTarget) {
        s_waiting = false;
        s_lastTarget = crosshairEntIndex;
    }

    if (!s_waiting) {
        s_triggerTime = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(TriggerBot::delayMs);
        s_waiting = true;
        return;
    }

    if (std::chrono::steady_clock::now() < s_triggerTime) return;

    s_waiting = false;
    s_lastTarget = -1;

    // Выстрел через SendInput
    INPUT input[2] = {};
    input[0].type = input[1].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
}