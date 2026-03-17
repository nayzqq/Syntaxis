#include "Movement.h"
#include "/test/GUI.h"
#include "/test/output/offsets.hpp"
#include "/test/output/client_dll.hpp"
#include "/test/Vector.h"
#include <Windows.h>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

// Оффсеты из buttons.cs (a2x/cs2-dumper 2026-03-12)
static constexpr uintptr_t BTN_JUMP = 0x2061E00;
static constexpr uintptr_t BTN_LEFT = 0x2061C50;
static constexpr uintptr_t BTN_RIGHT = 0x2061CE0;

// CS2: запись 65 = нажато, 0 = отпущено
static constexpr int BTN_PRESSED = 65;
static constexpr int BTN_RELEASED = 256;

static constexpr int FL_ONGROUND = (1 << 0);

// ─────────────────────────────────────────────────────────────────────────────
// BunnyHop
// Логика из reference movement.cpp:
// На земле с зажатым пробелом — сбрасываем кнопку прыжка (очищаем состояние)
// это заставляет движок прыгнуть ровно один тик при приземлении
// ─────────────────────────────────────────────────────────────────────────────
static void DoBunnyHop(uintptr_t client, uintptr_t localPawn)
{
    if (!Movement::bunnyHop) return;
    if (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) {
        *reinterpret_cast<int*>(client + BTN_JUMP) = BTN_RELEASED;
        return;
    }

    const int flags = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_fFlags);

    if (flags & FL_ONGROUND)
        // На земле: держим прыжок нажатым — движок прыгнет
        *reinterpret_cast<int*>(client + BTN_JUMP) = BTN_PRESSED;
    else
        // В воздухе: сбрасываем — чтобы движок не залипал
        *reinterpret_cast<int*>(client + BTN_JUMP) = BTN_RELEASED;
}

// ─────────────────────────────────────────────────────────────────────────────
// AutoStrafe
// ─────────────────────────────────────────────────────────────────────────────
static void DoAutoStrafe(uintptr_t client, uintptr_t localPawn)
{
    if (!Movement::autoStrafe) return;

    const int flags = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_fFlags);
    if (flags & FL_ONGROUND) {
        *reinterpret_cast<int*>(client + BTN_LEFT) = BTN_RELEASED;
        *reinterpret_cast<int*>(client + BTN_RIGHT) = BTN_RELEASED;
        return;
    }

    static POINT lastCursor = {};
    POINT curCursor = {};
    GetCursorPos(&curCursor);
    const int deltaX = curCursor.x - lastCursor.x;
    lastCursor = curCursor;

    if (deltaX == 0) {
        *reinterpret_cast<int*>(client + BTN_LEFT) = BTN_RELEASED;
        *reinterpret_cast<int*>(client + BTN_RIGHT) = BTN_RELEASED;
        return;
    }

    if (deltaX > 0) {
        *reinterpret_cast<int*>(client + BTN_RIGHT) = BTN_PRESSED;
        *reinterpret_cast<int*>(client + BTN_LEFT) = BTN_RELEASED;
    }
    else {
        *reinterpret_cast<int*>(client + BTN_LEFT) = BTN_PRESSED;
        *reinterpret_cast<int*>(client + BTN_RIGHT) = BTN_RELEASED;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Главная функция
// ─────────────────────────────────────────────────────────────────────────────
void RunMovement()
{
    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (*reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth) <= 0) return;

    // Лестница (8) и noclip (9) — пропускаем
    const int moveType = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_nActualMoveType);
    if (moveType == 8 || moveType == 9) return;

    DoBunnyHop(client, localPawn);
    DoAutoStrafe(client, localPawn);
}