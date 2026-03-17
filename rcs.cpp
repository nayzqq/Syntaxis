#include "RCS.h"
#include "/test/GUI.h"
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/Vector.h"
#include <Windows.h>
#include <cmath>

using namespace cs2_dumper::schemas::client_dll;

// m_aimPunchAngle = 0x16CC (QAngle = Vector3 pitch/yaw/roll)
static constexpr std::ptrdiff_t m_aimPunchAngle = 0x16CC;

// Запоминаем прошлый punch чтобы считать дельту (а не абсолютное значение)
static Vector3 s_lastPunch = {};

void RunRCS()
{
    if (!RCS::enabled) return;

    // RCS работает только когда зажата кнопка атаки (ЛКМ)
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        s_lastPunch = {};   // сброс при отпускании
        return;
    }

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (*reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth) <= 0) return;

    // Читаем текущий punch угол
    Vector3 punch = *reinterpret_cast<Vector3*>(localPawn + m_aimPunchAngle);

    // Дельта с прошлого кадра
    Vector3 delta;
    delta.x = punch.x - s_lastPunch.x;
    delta.y = punch.y - s_lastPunch.y;
    delta.z = 0.0f;
    s_lastPunch = punch;

    // Нет отдачи — ничего не делаем
    if (std::abs(delta.x) < 0.001f && std::abs(delta.y) < 0.001f) return;

    // Читаем текущие углы взгляда
    float* pViewAngles = reinterpret_cast<float*>(
        client + cs2_dumper::offsets::client_dll::dwViewAngles);
    if (!pViewAngles) return;

    // CS2 применяет punch * 2 к экрану, поэтому компенсируем * 2 * scale
    pViewAngles[0] -= delta.x * 2.0f * RCS::scaleX;
    pViewAngles[1] -= delta.y * 2.0f * RCS::scaleY;

    // Клемпим pitch
    if (pViewAngles[0] > 89.0f) pViewAngles[0] = 89.0f;
    if (pViewAngles[0] < -89.0f) pViewAngles[0] = -89.0f;

    // Нормализуем yaw
    while (pViewAngles[1] > 180.0f) pViewAngles[1] -= 360.0f;
    while (pViewAngles[1] < -180.0f) pViewAngles[1] += 360.0f;
}