#pragma once
#include "/test/Vector.h"

namespace Aimbot {
    // Основные настройки
    inline float aimFov = 5.0f;    // радиус FOV в градусах
    inline float smooth = 0.25f;   // плавность 0.01-1.0
    inline int   aimKey = 1;       // 1=ПКМ 2=CTRL 3=ALT 4=SHIFT 5=Mouse4
    inline int   boneTarget = 0;       // 0=голова 1=шея 2=грудь 3=живот 4=таз
    inline bool  showFov = false;   // показывать круг FOV
    inline bool  silentAim = false;   // мгновенное наведение (без сглаживания)

    // ── Autowall / Damage ─────────────────────────────────────────────────
    // Если damagePriority включён — цель выбирается по максимальному
    // расчётному урону (с учётом брони и хитгруппы), а не минимальному FOV.
    // Полезно для легита: аим идёт туда, где реально убьёт.
    inline bool  damagePriority = false;  // приоритизировать по урону
    inline float minDamage = 10.0f;  // не целиться если урон < N (autowall-фильтр)
    inline bool  autoWall = false;  // (заготовка) пытаться бить сквозь стены
}

void RunAimbot();
void DrawAimbotFov();