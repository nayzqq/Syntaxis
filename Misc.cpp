#include "Misc.h"
#include "/test/GUI.h"
#include "/test/Aimbot.h"
#include "/test/Movement.h"
#include "/test/RCS.h"
#include "/test/TriggerBot.h"
#include "/test/Radar.h"
#include "/test/SpectatorList.h"
#include "/test/BombTimer.h"
#include "/test/HitMarker.h"
#include "/test/imgui.h"
#include <Windows.h>
#include <ctime>
#include <stdio.h>
// ─────────────────────────────────────────────────────────────────────────────
// Panic Key
// Мгновенно выключает все фичи — всё в false, меню скрывается.
// Повторное нажатие восстанавливает предыдущее состояние.
// ─────────────────────────────────────────────────────────────────────────────

struct SavedState {
    bool box, skeleton, name, healthbar, weapon, reload, viewDirection;
    bool chams, teamchams;
    bool aimbot, silentAim, damagePriority;
    bool rcs;
    bool trigger;
    bool radar;
    bool spectator;
    bool bomb;
    bool hitmarker;
    bool bhop, autoStrafe;
    bool menu;
};

static SavedState s_saved = {};
static bool       s_panic = false;
static bool       s_lastKeyState = false;

static void SaveAll() {
    s_saved.box          = MyNamespace::visuals::box;
    s_saved.skeleton     = MyNamespace::visuals::skeleton;
    s_saved.name         = MyNamespace::visuals::name;
    s_saved.healthbar    = MyNamespace::visuals::healthbar;
    s_saved.weapon       = MyNamespace::visuals::weapon;
    s_saved.reload       = MyNamespace::visuals::reload;
    s_saved.viewDirection= MyNamespace::visuals::viewDirection;
    s_saved.chams        = MyNamespace::visuals::chams;
    s_saved.teamchams    = MyNamespace::visuals::teamchams;
    s_saved.aimbot       = MyNamespace::visuals::Aimbot;
    s_saved.rcs          = RCS::enabled;
    s_saved.trigger      = TriggerBot::enabled;
    s_saved.radar        = Radar::enabled;
    s_saved.spectator    = SpectatorList::enabled;
    s_saved.bomb         = BombTimer::enabled;
    s_saved.hitmarker    = HitMarker::enabled;
    s_saved.bhop         = Movement::bunnyHop;
    s_saved.autoStrafe   = Movement::autoStrafe;
    s_saved.menu         = MyNamespace::menuVisible;
}

static void DisableAll() {
    MyNamespace::visuals::box          = false;
    MyNamespace::visuals::skeleton     = false;
    MyNamespace::visuals::name         = false;
    MyNamespace::visuals::healthbar    = false;
    MyNamespace::visuals::weapon       = false;
    MyNamespace::visuals::reload       = false;
    MyNamespace::visuals::viewDirection= false;
    MyNamespace::visuals::chams        = false;
    MyNamespace::visuals::teamchams    = false;
    MyNamespace::visuals::Aimbot       = false;
    RCS::enabled                       = false;
    TriggerBot::enabled                = false;
    Radar::enabled                     = false;
    SpectatorList::enabled             = false;
    BombTimer::enabled                 = false;
    HitMarker::enabled                 = false;
    Movement::bunnyHop                 = false;
    Movement::autoStrafe               = false;
    MyNamespace::menuVisible           = false;
}

static void RestoreAll() {
    MyNamespace::visuals::box          = s_saved.box;
    MyNamespace::visuals::skeleton     = s_saved.skeleton;
    MyNamespace::visuals::name         = s_saved.name;
    MyNamespace::visuals::healthbar    = s_saved.healthbar;
    MyNamespace::visuals::weapon       = s_saved.weapon;
    MyNamespace::visuals::reload       = s_saved.reload;
    MyNamespace::visuals::viewDirection= s_saved.viewDirection;
    MyNamespace::visuals::chams        = s_saved.chams;
    MyNamespace::visuals::teamchams    = s_saved.teamchams;
    MyNamespace::visuals::Aimbot       = s_saved.aimbot;
    RCS::enabled                       = s_saved.rcs;
    TriggerBot::enabled                = s_saved.trigger;
    Radar::enabled                     = s_saved.radar;
    SpectatorList::enabled             = s_saved.spectator;
    BombTimer::enabled                 = s_saved.bomb;
    HitMarker::enabled                 = s_saved.hitmarker;
    Movement::bunnyHop                 = s_saved.bhop;
    Movement::autoStrafe               = s_saved.autoStrafe;
    MyNamespace::menuVisible           = s_saved.menu;
}

void RunMisc()
{
    if (!Misc::panicEnabled) return;

    const bool curKey = (GetAsyncKeyState(Misc::panicKey) & 0x8000) != 0;

    if (curKey && !s_lastKeyState) // rising edge
    {
        if (!s_panic) {
            SaveAll();
            DisableAll();
            s_panic = true;
        } else {
            RestoreAll();
            s_panic = false;
        }
    }
    s_lastKeyState = curKey;
}

// ─────────────────────────────────────────────────────────────────────────────
// Watermark
// ─────────────────────────────────────────────────────────────────────────────
void DrawWatermark()
{
    if (!Misc::watermark) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // FPS
    const float fps = ImGui::GetIO().Framerate;

    // Время
    time_t t = time(nullptr);
    tm tmBuf{};
    localtime_s(&tmBuf, &t);
    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &tmBuf);

    char buf[128];
    snprintf(buf, sizeof(buf), "CS2 Cheat  |  %.0f FPS  |  %s", fps, timeBuf);

    const ImVec2 sz  = ImGui::CalcTextSize(buf);
    const float  pad = 6.0f;
    const float  x   = 10.0f;
    const float  y   = 10.0f;

    dl->AddRectFilled(
        ImVec2(x - pad, y - pad),
        ImVec2(x + sz.x + pad, y + sz.y + pad),
        IM_COL32(0, 0, 0, 150), 3.0f);

    dl->AddRect(
        ImVec2(x - pad, y - pad),
        ImVec2(x + sz.x + pad, y + sz.y + pad),
        IM_COL32(80, 80, 80, 180), 3.0f, 0, 1.0f);

    dl->AddText(ImVec2(x, y), IM_COL32(200, 200, 200, 255), buf);
}
