#pragma once
#include <cstdint>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Хитгруппы CS2
// ─────────────────────────────────────────────────────────────────────────────
enum EHitgroups : int
{
    HITGROUP_GENERIC = 0,
    HITGROUP_HEAD = 1,
    HITGROUP_CHEST = 2,
    HITGROUP_STOMACH = 3,
    HITGROUP_LEFTARM = 4,
    HITGROUP_RIGHTARM = 5,
    HITGROUP_LEFTLEG = 6,
    HITGROUP_RIGHTLEG = 7,
    HITGROUP_NECK = 8,
    HITGROUP_GEAR = 10,
};

// ─────────────────────────────────────────────────────────────────────────────
// Параметры оружия (заполняются из memory read по offsets)
// ─────────────────────────────────────────────────────────────────────────────
struct WeaponParams
{
    float damage = 100.0f;
    float headshot_mult = 4.0f;   // обычно 4x для большинства оружий
    float armor_ratio = 0.5f;   // AWP=1.0, пистолеты~0.5, rifles~0.5-0.57
    float range = 8192.0f;
    float range_modifier = 0.99f;  // затухание урона на дистанции
    float penetration = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// Статус цели
// ─────────────────────────────────────────────────────────────────────────────
struct TargetArmorInfo
{
    int  armor_value = 0;
    bool has_helmet = false;
    bool has_heavy = false;  // CZ-taser tier heavy armor
    int  team = 2;      // 2=T, 3=CT
};

// ─────────────────────────────────────────────────────────────────────────────
// Урон по дистанции (из FireBullet — затухание range_modifier ^ (dist/500))
// ─────────────────────────────────────────────────────────────────────────────
inline float ApplyRangeFalloff(float base_dmg, float range_modifier, float distance)
{
    return base_dmg * std::powf(range_modifier, distance / 500.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Скейлинг урона по хитгруппе + броне
// Логика 1-в-1 из penetration.cpp::ScaleDamage2
// ─────────────────────────────────────────────────────────────────────────────
inline float ScaleDamageByHitgroup(
    float          base_dmg,
    int            hitgroup,
    const WeaponParams& wpn,
    const TargetArmorInfo& armor)
{
    // Командные коэффициенты (стандартные серверные значения, можно читать из ConVar)
    const float ct_head_scale = 1.0f, t_head_scale = 1.0f;
    const float ct_body_scale = 1.0f, t_body_scale = 1.0f;

    const bool is_ct = (armor.team == 3);
    const bool is_t = (armor.team == 2);

    float head_scale = is_ct ? ct_head_scale : (is_t ? t_head_scale : 1.0f);
    float body_scale = is_ct ? ct_body_scale : (is_t ? t_body_scale : 1.0f);

    if (armor.has_heavy)
        head_scale *= 0.5f;

    // Хитгрупп-мультипликаторы
    switch (hitgroup)
    {
    case HITGROUP_HEAD:
        base_dmg *= wpn.headshot_mult * head_scale;
        break;
    case HITGROUP_STOMACH:
        base_dmg *= 1.25f * body_scale;
        break;
    case HITGROUP_LEFTLEG:
    case HITGROUP_RIGHTLEG:
        base_dmg *= 0.75f * body_scale;
        break;
    case HITGROUP_CHEST:
    case HITGROUP_LEFTARM:
    case HITGROUP_RIGHTARM:
    case HITGROUP_NECK:
    default:
        base_dmg *= body_scale;
        break;
    }

    // Броня (есть только на голове с шлемом и теле)
    const bool has_armor =
        (hitgroup == HITGROUP_HEAD && armor.has_helmet) ||
        (hitgroup != HITGROUP_HEAD && hitgroup != HITGROUP_LEFTLEG &&
            hitgroup != HITGROUP_RIGHTLEG && armor.armor_value > 0);

    if (!has_armor)
        return base_dmg;

    float heavy_bonus = 1.0f, armor_bonus = 0.5f;
    float armor_ratio = wpn.armor_ratio * 0.5f;

    if (armor.has_heavy)
    {
        heavy_bonus = 0.25f;
        armor_bonus = 0.33f;
        armor_ratio *= 0.20f;
    }

    float dmg_to_health = base_dmg * armor_ratio;
    const float dmg_to_armor = (base_dmg - dmg_to_health) * (heavy_bonus * armor_bonus);

    if (dmg_to_armor > static_cast<float>(armor.armor_value))
        dmg_to_health = base_dmg - static_cast<float>(armor.armor_value) / armor_bonus;

    return dmg_to_health;
}

// ─────────────────────────────────────────────────────────────────────────────
// Получить хитгруппу по индексу кости (для boneTarget из Aimbot)
// ─────────────────────────────────────────────────────────────────────────────
inline int BoneTargetToHitgroup(int boneTarget)
{
    switch (boneTarget)
    {
    case 0: return HITGROUP_HEAD;
    case 1: return HITGROUP_NECK;
    case 2: return HITGROUP_CHEST;
    case 3: return HITGROUP_STOMACH;
    case 4: return HITGROUP_LEFTLEG;
    default: return HITGROUP_CHEST;
    }
}