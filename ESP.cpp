#include "windows.h"
#include <cstdint>
#include <cstring>          // для strncpy_s
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/Vector.h"
#include <cmath>
#include <optional>
#include <unordered_map>
#include "/test/imgui.h"
#include "/test/GUI.h"
#include "/test/Aimbot.h"
#include "/test/Misc.h"

// Константы
static constexpr std::ptrdiff_t BONE_ARRAY_IN_MODELSTATE = 0x80;


static void NormalizeAngles(Vector3& a) {
    while (a.y > 180.0f) a.y -= 360.0f;
    while (a.y < -180.0f) a.y += 360.0f;
    if (a.x > 89.0f) a.x = 89.0f;
    if (a.x < -89.0f) a.x = -89.0f;
    a.z = 0.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Маппинг ItemDefinitionIndex -> название оружия
// ─────────────────────────────────────────────────────────────────────────────
static const std::unordered_map<uint16_t, const char*> g_WeaponNames = {
    { 1,  "Desert Eagle" }, { 2,  "Dual Berettas" }, { 3,  "Five-SeveN"  },
    { 4,  "Glock-18"     }, { 7,  "AK-47"         }, { 8,  "AUG"         },
    { 9,  "AWP"          }, { 10, "FAMAS"          }, { 11, "G3SG1"       },
    { 13, "Galil AR"     }, { 14, "M249"           }, { 16, "M4A4"        },
    { 17, "MAC-10"       }, { 19, "P90"            }, { 20, "MP5-SD"      },
    { 23, "MP7"          }, { 24, "UMP-45"         }, { 25, "XM1014"      },
    { 26, "PP-Bizon"     }, { 27, "MAG-7"          }, { 28, "Negev"       },
    { 29, "Sawed-Off"    }, { 30, "Tec-9"          }, { 31, "Zeus x27"    },
    { 32, "P2000"        }, { 33, "MP9"            }, { 34, "Nova"        },
    { 35, "P250"         }, { 38, "SSG 08"         }, { 39, "Knife (CT)"  },
    { 40, "Knife (T)"    }, { 41, "Flashbang"      }, { 42, "HE Grenade"  },
    { 43, "Smoke"        }, { 44, "Molotov"        }, { 45, "Decoy"       },
    { 46, "Incendiary"   }, { 47, "C4"             }, { 60, "M4A1-S"      },
    { 61, "USP-S"        }, { 63, "CZ75-Auto"      }, { 64, "R8 Revolver" },
    { 68, "SG 553"       },
};

static const char* GetWeaponName(uint16_t def_index) {
    auto it = g_WeaponNames.find(def_index);
    return (it != g_WeaponNames.end()) ? it->second : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Кости и соединения
// ─────────────────────────────────────────────────────────────────────────────
static const int g_BoneA[] = { 0,  2,  4,  5,  6,  6,  8,  9, 10,  6, 13, 14, 15,  0, 22, 23,  0, 26, 27 };
static const int g_BoneB[] = { 2,  4,  5,  6,  7,  8,  9, 10, 11, 13, 14, 15, 16, 22, 23, 24, 26, 27, 28 };
static const int g_BoneCount = 19;
static const int BONE_MAX = 32;

// ─────────────────────────────────────────────────────────────────────────────
// Утилиты для работы с сущностями
// ─────────────────────────────────────────────────────────────────────────────
uintptr_t GetBaseEntity(int index, uintptr_t client) {
    auto base = *reinterpret_cast<std::uintptr_t*>(
        client + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!base) return 0;
    auto elb = *reinterpret_cast<std::uintptr_t*>(base + 0x8 * (index >> 9) + 16);
    if (!elb) return 0;
    return *reinterpret_cast<std::uintptr_t*>(elb + 0x70 * (index & 0x1FF));
}

uintptr_t GetBaseEntityFromHandle(uint32_t uHandle, uintptr_t client) {
    auto base = *reinterpret_cast<std::uintptr_t*>(
        client + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!base) return 0;
    const int nIndex = uHandle & 0x7FFF;
    auto elb = *reinterpret_cast<std::uintptr_t*>(base + 8 * (nIndex >> 9) + 16);
    if (!elb) return 0;
    return *reinterpret_cast<std::uintptr_t*>(elb + 0x70 * (nIndex & 0x1FF));
}

std::optional<Vector3> GetEyePos(uintptr_t addr) noexcept {
    using namespace cs2_dumper::schemas::client_dll;
    auto* origin = reinterpret_cast<Vector3*>(addr + C_BasePlayerPawn::m_vOldOrigin);
    auto* offset = reinterpret_cast<Vector3*>(addr + C_BaseModelEntity::m_vecViewOffset);
    Vector3 eye = *origin + *offset;
    if (!std::isfinite(eye.x) || !std::isfinite(eye.y) || !std::isfinite(eye.z))
        return std::nullopt;
    if (eye.Length() < 0.1f)
        return std::nullopt;
    return eye;
}

bool WorldToScreen(Vector3 world, Vector3& screen, float* matrix, float sw, float sh) {
    float m[4][4];
    memcpy(m, matrix, 16 * sizeof(float));
    const float w = m[3][0] * world.x + m[3][1] * world.y + m[3][2] * world.z + m[3][3];
    if (w < 0.01f) return false;
    const float x = m[0][0] * world.x + m[0][1] * world.y + m[0][2] * world.z + m[0][3];
    const float y = m[1][0] * world.x + m[1][1] * world.y + m[1][2] * world.z + m[1][3];
    screen.x = (sw / 2.0f) * (x / w + 1.0f);
    screen.y = (sh / 2.0f) * (1.0f - y / w);
    screen.z = 0.0f;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Основная функция отрисовки ESP
// ─────────────────────────────────────────────────────────────────────────────
void draw_esp() {
    DrawAimbotFov(); // рисуем круг FOV если включён

    static auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    const float screen_w = ImGui::GetIO().DisplaySize.x;
    const float screen_h = ImGui::GetIO().DisplaySize.y;
    if (screen_w <= 0.0f || screen_h <= 0.0f) return;

    using namespace cs2_dumper::schemas::client_dll;

    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
        client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;
    if (*reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth) <= 0) return;
    const int localTeam = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iTeamNum);

    float* viewMatrix = reinterpret_cast<float*>(
        client + cs2_dumper::offsets::client_dll::dwViewMatrix);
    if (!viewMatrix) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    if (!dl) return;

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
        const int hp = *reinterpret_cast<int*>(pawn + C_BaseEntity::m_iHealth);
        if (team == localTeam || hp <= 0) continue;

        Vector3 origin = *reinterpret_cast<Vector3*>(
            pawn + C_BasePlayerPawn::m_vOldOrigin);

        auto eye_opt = GetEyePos(pawn);
        if (!eye_opt) continue;

        Vector3 head_2d, origin_2d;
        if (!WorldToScreen(*eye_opt, head_2d, viewMatrix, screen_w, screen_h)) continue;
        if (!WorldToScreen(origin, origin_2d, viewMatrix, screen_w, screen_h)) continue;

        // Геометрия бокса
        const float cx = (head_2d.x + origin_2d.x) * 0.5f;
        const float body_height = origin_2d.y - head_2d.y;
        const float box_top = head_2d.y - body_height * 0.16f;
        const float box_bottom = origin_2d.y + body_height * 0.05f;
        const float height = box_bottom - box_top;
        const float width = height / 1.9f;
        const float left = cx - width * 0.5f;
        const float right = cx + width * 0.5f;
        if (height <= 0.0f || width <= 0.0f) continue;

        // Оружие
        uint16_t   weapon_defid = 0;
        bool       is_reloading = false;
        uintptr_t  weapon_ent = 0;

        uintptr_t weapon_services = *reinterpret_cast<uintptr_t*>(
            pawn + C_BasePlayerPawn::m_pWeaponServices);

        if (weapon_services)
        {
            uint32_t hWeapon = *reinterpret_cast<uint32_t*>(
                weapon_services + CPlayer_WeaponServices::m_hActiveWeapon);

            if (hWeapon && hWeapon != 0xFFFFFFFF)
            {
                weapon_ent = GetBaseEntityFromHandle(hWeapon, client);
                if (weapon_ent)
                {
                    weapon_defid = *reinterpret_cast<uint16_t*>(
                        weapon_ent
                        + C_EconEntity::m_AttributeManager
                        + C_AttributeContainer::m_Item
                        + C_EconItemView::m_iItemDefinitionIndex);

                    is_reloading = *reinterpret_cast<bool*>(
                        weapon_ent + C_CSWeaponBase::m_bInReload);
                }
            }
        }

        if (MyNamespace::visuals::box)
        {
            const float clen = width * 0.25f; // длина угловых линий = 25% ширины бокса
            ImU32 boxCol = IM_COL32(255, 0, 0, 255);

            if (Misc::cornerBox)
            {
                // Верхний левый
                dl->AddLine(ImVec2(left, box_top), ImVec2(left + clen, box_top), boxCol, 1.5f);
                dl->AddLine(ImVec2(left, box_top), ImVec2(left, box_top + clen), boxCol, 1.5f);
                // Верхний правый
                dl->AddLine(ImVec2(right, box_top), ImVec2(right - clen, box_top), boxCol, 1.5f);
                dl->AddLine(ImVec2(right, box_top), ImVec2(right, box_top + clen), boxCol, 1.5f);
                // Нижний левый
                dl->AddLine(ImVec2(left, box_bottom), ImVec2(left + clen, box_bottom), boxCol, 1.5f);
                dl->AddLine(ImVec2(left, box_bottom), ImVec2(left, box_bottom - clen), boxCol, 1.5f);
                // Нижний правый
                dl->AddLine(ImVec2(right, box_bottom), ImVec2(right - clen, box_bottom), boxCol, 1.5f);
                dl->AddLine(ImVec2(right, box_bottom), ImVec2(right, box_bottom - clen), boxCol, 1.5f);
            }
            else
            {
                dl->AddRect(ImVec2(left, box_top), ImVec2(right, box_bottom),
                    boxCol, 0.0f, 0, 1.5f);
            }
        }


        if (MyNamespace::visuals::name)
        {
            const char* raw = reinterpret_cast<const char*>(
                controller + CBasePlayerController::m_iszPlayerName);
            char name_buf[128] = "unknown";
            if (raw && raw[0] != '\0')
                strncpy_s(name_buf, raw, sizeof(name_buf) - 1);
            const ImVec2 sz = ImGui::CalcTextSize(name_buf);
            dl->AddText(ImVec2(cx - sz.x * 0.5f, box_top - sz.y - 2.0f),
                IM_COL32(255, 255, 255, 255), name_buf);
        }

        if (MyNamespace::visuals::weapon && weapon_defid != 0)
        {
            const char* label = GetWeaponName(weapon_defid);
            if (label)
            {
                const ImVec2 sz = ImGui::CalcTextSize(label);
                dl->AddText(ImVec2(cx - sz.x * 0.5f, box_bottom + 2.0f),
                    IM_COL32(255, 220, 0, 255), label);
            }
        }

        if (MyNamespace::visuals::reload && is_reloading)
        {
            const char* reload_text = "[ RELOADING ]";
            const ImVec2 sz = ImGui::CalcTextSize(reload_text);
            const float  y_off = MyNamespace::visuals::weapon ? 16.0f : 2.0f;
            dl->AddText(ImVec2(cx - sz.x * 0.5f, box_bottom + y_off),
                IM_COL32(255, 80, 80, 255), reload_text);
        }

        if (MyNamespace::visuals::skeleton)
        {
            uintptr_t scene_node = *reinterpret_cast<uintptr_t*>(
                pawn + C_BaseEntity::m_pGameSceneNode);
            if (!scene_node) continue;

            uintptr_t bone_array = *reinterpret_cast<uintptr_t*>(
                scene_node + CSkeletonInstance::m_modelState + BONE_ARRAY_IN_MODELSTATE);
            if (!bone_array) continue;

            Vector3 bones_2d[BONE_MAX];
            bool    bones_ok[BONE_MAX] = { false };

            for (int j = 0; j < g_BoneCount; j++)
            {
                int a = g_BoneA[j];
                int b = g_BoneB[j];
                if (!bones_ok[a]) {
                    Vector3 bw = *reinterpret_cast<Vector3*>(bone_array + a * 32);
                    bones_ok[a] = WorldToScreen(bw, bones_2d[a], viewMatrix, screen_w, screen_h);
                }
                if (!bones_ok[b]) {
                    Vector3 bw = *reinterpret_cast<Vector3*>(bone_array + b * 32);
                    bones_ok[b] = WorldToScreen(bw, bones_2d[b], viewMatrix, screen_w, screen_h);
                }
            }

            for (int j = 0; j < g_BoneCount; j++)
            {
                int a = g_BoneA[j];
                int b = g_BoneB[j];
                if (!bones_ok[a] || !bones_ok[b]) continue;
                dl->AddLine(ImVec2(bones_2d[a].x, bones_2d[a].y),
                    ImVec2(bones_2d[b].x, bones_2d[b].y),
                    IM_COL32(255, 255, 255, 200), 1.0f);
            }
        }

        // Направление взгляда (View Direction) – добавлено
        if (MyNamespace::visuals::viewDirection)
        {
            // Используем C_CSPlayerPawn::m_angEyeAngles (смещение 0x3DD0)
            Vector3 eyeAngles = *reinterpret_cast<Vector3*>(pawn + C_CSPlayerPawn::m_angEyeAngles);
            NormalizeAngles(eyeAngles);

            float pitch = Deg2Rad(eyeAngles.x);
            float yaw = Deg2Rad(eyeAngles.y);
            Vector3 forward;
            forward.x = cos(pitch) * cos(yaw);
            forward.y = cos(pitch) * sin(yaw);
            forward.z = -sin(pitch);

            Vector3 endPos = *eye_opt + forward * 2000.0f;

            Vector3 start2d, end2d;
            if (WorldToScreen(*eye_opt, start2d, viewMatrix, screen_w, screen_h) &&
                WorldToScreen(endPos, end2d, viewMatrix, screen_w, screen_h))
            {
                dl->AddLine(ImVec2(start2d.x, start2d.y), ImVec2(end2d.x, end2d.y),
                    IM_COL32(255, 255, 0, 200), 1.5f);
            }
        }
    }
}