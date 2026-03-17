#include "Radar.h"
#include "/test/GUI.h"
#include "/test/ESP.h"
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/Vector.h"
#include "/test/imgui.h"
#include <Windows.h>
#include <cmath>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

void DrawRadar()
{
    if (!Radar::enabled) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (*reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth) <= 0) return;

    const int localTeam = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iTeamNum);

    Vector3 localOrigin = *reinterpret_cast<Vector3*>(
        localPawn + C_BasePlayerPawn::m_vOldOrigin);

    float* pViewAngles = reinterpret_cast<float*>(
        client + client_dll::dwViewAngles);
    const float localYaw = pViewAngles ? pViewAngles[1] : 0.0f;

    // ИСПРАВЛЕНО: правильный перевод в радианы для системы координат CS2
    // В CS2: yaw=0 смотрим на +X, yaw=90 смотрим на +Y
    const float yawRad = localYaw * 3.141592653f / 180.0f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float half = Radar::size * 0.5f;
    const float cx = Radar::posX + half;
    const float cy = Radar::posY + half;

    // Фон
    dl->AddRectFilled(
        ImVec2(Radar::posX, Radar::posY),
        ImVec2(Radar::posX + Radar::size, Radar::posY + Radar::size),
        IM_COL32(0, 0, 0, 160), 4.0f);
    dl->AddRect(
        ImVec2(Radar::posX, Radar::posY),
        ImVec2(Radar::posX + Radar::size, Radar::posY + Radar::size),
        IM_COL32(80, 80, 80, 200), 4.0f, 0, 1.5f);

    // Концентрические окружности
    dl->AddCircle(ImVec2(cx, cy), half * 0.33f, IM_COL32(50, 50, 50, 200), 32, 1.0f);
    dl->AddCircle(ImVec2(cx, cy), half * 0.66f, IM_COL32(50, 50, 50, 200), 32, 1.0f);

    // Локальный игрок — стрелка вверх (всегда смотрит "вперёд" = вверх радара)
    dl->AddLine(ImVec2(cx - 4, cy + 3), ImVec2(cx, cy - 7), IM_COL32(0, 255, 0, 255), 1.5f);
    dl->AddLine(ImVec2(cx + 4, cy + 3), ImVec2(cx, cy - 7), IM_COL32(0, 255, 0, 255), 1.5f);
    dl->AddLine(ImVec2(cx - 4, cy + 3), ImVec2(cx + 4, cy + 3), IM_COL32(0, 255, 0, 255), 1.5f);

    // Враги
    for (int i = 0; i < 64; i++)
    {
        uintptr_t controller = GetBaseEntity(i, client);
        if (!controller) continue;

        uint32_t hPawn = *reinterpret_cast<uint32_t*>(
            controller + CBasePlayerController::m_hPawn);
        if (!hPawn) continue;

        uintptr_t pawn = GetBaseEntityFromHandle(hPawn, client);
        if (!pawn || pawn == localPawn) continue;

        const int team = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iTeamNum);
        const int health = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iHealth);
        if (health <= 0) continue;

        Vector3 enemyOrigin = *reinterpret_cast<Vector3*>(
            pawn + C_BasePlayerPawn::m_vOldOrigin);

        // Дельта в мировых координатах
        const float dx = enemyOrigin.x - localOrigin.x;
        const float dy = enemyOrigin.y - localOrigin.y;

        // ИСПРАВЛЕНО: правильное вращение координат для CS2
        // forward = (cos(yaw), sin(yaw)) — направление взгляда
        // right   = (sin(yaw), -cos(yaw)) — правая сторона
        //
        // radar_x = проекция дельты на right   → X экрана (вправо)
        // radar_y = проекция дельты на forward → Y экрана (вверх = минус screen Y)
        const float cosY = std::cos(yawRad);
        const float sinY = std::sin(yawRad);

        const float radar_x = dx * sinY - dy * cosY;  // правая компонента
        const float radar_y = dx * cosY + dy * sinY;  // передняя компонента

        float sx = cx + (radar_x / Radar::range) * half;
        float sy = cy - (radar_y / Radar::range) * half; // инверт Y

        // Клемп к краю если вне радиуса
        const float relX = sx - cx, relY = sy - cy;
        const float dist = std::sqrt(relX * relX + relY * relY);
        if (dist > half - 4.0f) {
            const float scale = (half - 4.0f) / dist;
            sx = cx + relX * scale;
            sy = cy + relY * scale;
        }

        // Цвет: враги красные, союзники зелёные
        const ImU32 color = (team == localTeam)
            ? IM_COL32(0, 255, 100, 255)
            : IM_COL32(255, 50, 50, 255);

        dl->AddCircleFilled(ImVec2(sx, sy), 3.5f, color);

        if (Radar::showNames) {
            const char* raw = reinterpret_cast<const char*>(
                controller + CBasePlayerController::m_iszPlayerName);
            if (raw && raw[0])
                dl->AddText(ImVec2(sx + 5.0f, sy - 6.0f),
                    IM_COL32(255, 255, 255, 180), raw);
        }
    }

    // Подпись дистанции
    char label[32];
    sprintf_s(label, "%.0fm", Radar::range / 100.0f);
    dl->AddText(ImVec2(Radar::posX + 4.0f, Radar::posY + Radar::size - 14.0f),
        IM_COL32(120, 120, 120, 200), label);
}