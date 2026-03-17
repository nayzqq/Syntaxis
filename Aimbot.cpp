#include "Aimbot.h"
#include "/test/ESP.h"
#include "/test/GUI.h"
#include "/test/DamageScale.h"
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/Vector.h"
#include "/test/imgui.h"
#include <cmath>
#include <Windows.h>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

// Оффсеты из client_dll.cs (автодамп)
static constexpr std::ptrdiff_t m_bHasHelmet_off = 0x49;   // C_CSPlayerPawnBase
static constexpr std::ptrdiff_t m_ArmorValue_off = 0x272C; // C_CSPlayerPawn

static Vector3 CalcAngle(const Vector3& eye, const Vector3& target)
{
    Vector3 delta = target - eye;
    float   dist2d = delta.Length2D();
    if (dist2d < 1e-6f) dist2d = 1e-6f;
    return Vector3(-Rad2Deg(std::atan2(delta.z, dist2d)),
        Rad2Deg(std::atan2(delta.y, delta.x)), 0.0f);
}

static void NormalizeAngles(Vector3& a)
{
    while (a.y > 180.0f) a.y -= 360.0f;
    while (a.y < -180.0f) a.y += 360.0f;
    a.x = std::fmax(-89.0f, std::fmin(89.0f, a.x));
    a.z = 0.0f;
}

static float AngleDiff(const Vector3& a, const Vector3& b)
{
    float dp = std::abs(a.x - b.x);
    float dy = std::abs(a.y - b.y);
    if (dy > 180.0f) dy = 360.0f - dy;
    return std::sqrt(dp * dp + dy * dy);
}

static const int g_Bones[] = { 7, 6, 5, 4, 3 };

static bool GetAimPos(uintptr_t pawn, Vector3& outPos)
{
    if (Aimbot::boneTarget == 0) {
        auto opt = GetEyePos(pawn);
        if (opt.has_value()) { outPos = opt.value(); return true; }
    }
    const int boneIdx = g_Bones[Aimbot::boneTarget < 5 ? Aimbot::boneTarget : 0];
    uintptr_t scene_node = *reinterpret_cast<uintptr_t*>(
        pawn + C_BaseEntity::m_pGameSceneNode);
    if (!scene_node) return false;
    uintptr_t bone_array = *reinterpret_cast<uintptr_t*>(
        scene_node + CSkeletonInstance::m_modelState + 0x80);
    if (!bone_array) return false;
    outPos = *reinterpret_cast<Vector3*>(bone_array + boneIdx * 32);
    return (std::isfinite(outPos.x) && std::isfinite(outPos.y) && std::isfinite(outPos.z));
}

static bool IsAimKeyDown()
{
    switch (Aimbot::aimKey) {
    case 1: return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    case 2: return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    case 3: return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    case 4: return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    case 5: return (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
    default: return false;
    }
}

// ИСПРАВЛЕНО: читаем броню с правильными оффсетами из client_dll.cs
static TargetArmorInfo ReadArmorInfo(uintptr_t pawn)
{
    TargetArmorInfo info;
    info.team = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iTeamNum);
    info.armor_value = *reinterpret_cast<int*>(pawn + m_ArmorValue_off);
    info.has_helmet = *reinterpret_cast<bool*>(pawn + m_bHasHelmet_off);
    info.has_heavy = false;
    return info;
}

static float CalcExpectedDamage(const Vector3& eye, const Vector3& pos,
    int boneTarget, const WeaponParams& wpn,
    const TargetArmorInfo& armor)
{
    const float dist = (pos - eye).Length();
    const float dmg = ApplyRangeFalloff(wpn.damage, wpn.range_modifier, dist);
    return ScaleDamageByHitgroup(dmg, BoneTargetToHitgroup(boneTarget), wpn, armor);
}

void DrawAimbotFov()
{
    if (!Aimbot::showFov || !MyNamespace::visuals::Aimbot) return;
    const float sw = ImGui::GetIO().DisplaySize.x;
    const float sh = ImGui::GetIO().DisplaySize.y;
    const float fovPx = (Aimbot::aimFov / 90.0f) * (sw * 0.5f);
    ImGui::GetForegroundDrawList()->AddCircle(
        ImVec2(sw * 0.5f, sh * 0.5f), fovPx,
        IM_COL32(255, 255, 255, 120), 64, 1.0f);
}

void RunAimbot()
{
    if (!MyNamespace::visuals::Aimbot) return;
    if (!IsAimKeyDown()) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (*reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth) <= 0) return;

    auto localEyeOpt = GetEyePos(localPawn);
    if (!localEyeOpt.has_value()) return;
    const Vector3 localEye = localEyeOpt.value();

    float* pViewAngles = reinterpret_cast<float*>(
        client + client_dll::dwViewAngles);
    if (!pViewAngles) return;
    Vector3 viewAngles(pViewAngles[0], pViewAngles[1], 0.0f);

    const int localTeam = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iTeamNum);

    // Дефолтные параметры оружия (можно расширить чтением VData)
    WeaponParams wpn;

    float   bestFov = Aimbot::aimFov;
    float   bestDamage = Aimbot::minDamage;
    Vector3 bestPos = {};
    bool    found = false;

    for (int i = 0; i < 64; i++)
    {
        uintptr_t controller = GetBaseEntity(i, client);
        if (!controller) continue;

        uint32_t hPawn = *reinterpret_cast<uint32_t*>(
            controller + CBasePlayerController::m_hPawn);
        if (!hPawn) continue;

        uintptr_t pawn = GetBaseEntityFromHandle(hPawn, client);
        if (!pawn || pawn == (uintptr_t)-1 || pawn == localPawn) continue;

        const int team = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iTeamNum);
        const int health = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iHealth);
        if (team == localTeam || health <= 0) continue;

        Vector3 aimPos;
        if (!GetAimPos(pawn, aimPos)) continue;

        const float fov = AngleDiff(viewAngles, CalcAngle(localEye, aimPos));
        if (fov > Aimbot::aimFov) continue;

        if (Aimbot::damagePriority)
        {
            // ИСПРАВЛЕНО: читаем броню с правильными оффсетами
            const TargetArmorInfo armor = ReadArmorInfo(pawn);
            const float dmg = CalcExpectedDamage(localEye, aimPos,
                Aimbot::boneTarget, wpn, armor);
            if (dmg < Aimbot::minDamage) continue;
            if (dmg > bestDamage) {
                bestDamage = dmg; bestPos = aimPos; found = true;
            }
        }
        else
        {
            if (fov < bestFov) {
                bestFov = fov; bestPos = aimPos; found = true;
            }
        }
    }

    if (!found) return;

    Vector3 targetAngles = CalcAngle(localEye, bestPos);
    NormalizeAngles(targetAngles);

    if (!Aimbot::silentAim)
    {
        float t = std::fmax(0.01f, std::fmin(1.0f, Aimbot::smooth));
        float dx = targetAngles.x - viewAngles.x;
        float dy = targetAngles.y - viewAngles.y;
        while (dy > 180.0f) dy -= 360.0f;
        while (dy < -180.0f) dy += 360.0f;

        Vector3 result(viewAngles.x + dx * t, viewAngles.y + dy * t, 0.0f);
        NormalizeAngles(result);
        pViewAngles[0] = result.x;
        pViewAngles[1] = result.y;
    }
    else
    {
        pViewAngles[0] = targetAngles.x;
        pViewAngles[1] = targetAngles.y;
    }
}