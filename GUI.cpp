#include "GUI.h"
#include "/test/Aimbot.h"
#include "/test/Movement.h"
#include "/test/RCS.h"
#include "/test/TriggerBot.h"
#include "/test/Radar.h"
#include "/test/SpectatorList.h"
#include "/test/BombTimer.h"
#include "/test/HitMarker.h"
#include "/test/Misc.h"
#include "/test/Config.h"
#include "/test/imgui.h"

void draw_Menu()
{
    ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Once);
    ImGui::Begin("CS2 External", nullptr, ImGuiWindowFlags_NoCollapse);

    // ── ESP ───────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Box", &MyNamespace::visuals::box);
        ImGui::Checkbox("Skeleton", &MyNamespace::visuals::skeleton);
        ImGui::Checkbox("Name", &MyNamespace::visuals::name);
        ImGui::Checkbox("Health Bar", &MyNamespace::visuals::healthbar);
        ImGui::Checkbox("Weapon", &MyNamespace::visuals::weapon);
        ImGui::Checkbox("Reload", &MyNamespace::visuals::reload);
        ImGui::Checkbox("View Direction", &MyNamespace::visuals::viewDirection);
    }

    // ── Chams ─────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Chams"))
    {
        ImGui::Checkbox("Chams (синий/жёлтый)", &MyNamespace::visuals::chams);
        ImGui::Checkbox("TeamChams (T=красный CT=зелёный)", &MyNamespace::visuals::teamchams);
    }

    // ── Aimbot ────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Enable", &MyNamespace::visuals::Aimbot);
        if (MyNamespace::visuals::Aimbot)
        {
            ImGui::Separator();
            static const char* keys[] = { "RMB", "CTRL", "ALT", "SHIFT", "Mouse4" };
            static const char* bones[] = { "Head", "Neck", "Chest", "Stomach", "Pelvis" };
            int keyIdx = Aimbot::aimKey - 1;
            if (ImGui::Combo("Key", &keyIdx, keys, 5)) Aimbot::aimKey = keyIdx + 1;
            ImGui::Combo("Target", &Aimbot::boneTarget, bones, IM_ARRAYSIZE(bones));
            ImGui::SliderFloat("FOV", &Aimbot::aimFov, 0.5f, 45.0f, "%.1f");
            ImGui::SliderFloat("Smooth", &Aimbot::smooth, 0.01f, 1.0f, "%.2f");
            ImGui::Checkbox("Show FOV Circle", &Aimbot::showFov);
            ImGui::Checkbox("Silent Aim", &Aimbot::silentAim);
            ImGui::Separator();
            ImGui::Checkbox("Damage Priority", &Aimbot::damagePriority);
            if (Aimbot::damagePriority)
                ImGui::SliderFloat("Min Damage", &Aimbot::minDamage, 1.0f, 100.0f, "%.0f");
        }
    }

    // ── RCS ───────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("RCS"))
    {
        ImGui::Checkbox("Enable##rcs", &RCS::enabled);
        if (RCS::enabled)
        {
            ImGui::SliderFloat("Scale X##rcs", &RCS::scaleX, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Scale Y##rcs", &RCS::scaleY, 0.0f, 2.0f, "%.2f");
            ImGui::TextDisabled("  1.0 = полная компенсация");
        }
    }

    // ── TriggerBot ────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("TriggerBot"))
    {
        ImGui::Checkbox("Enable##tb", &TriggerBot::enabled);
        if (TriggerBot::enabled)
        {
            ImGui::Separator();
            static const char* tbkeys[] = { "Always", "ALT", "SHIFT", "Mouse4" };
            ImGui::Combo("Key##tb", &TriggerBot::hotkey, tbkeys, IM_ARRAYSIZE(tbkeys));
            ImGui::SliderInt("Delay ms", &TriggerBot::delayMs, 0, 300);
            ImGui::Checkbox("Headshot Only", &TriggerBot::headshotOnly);
            ImGui::TextDisabled("  Стреляет когда прицел на враге");
        }
    }

    // ── Radar ─────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Radar"))
    {
        ImGui::Checkbox("Enable##radar", &Radar::enabled);
        if (Radar::enabled)
        {
            ImGui::SliderFloat("Size##radar", &Radar::size, 100.0f, 400.0f, "%.0f px");
            ImGui::SliderFloat("Range##radar", &Radar::range, 500.0f, 5000.0f, "%.0f");
            ImGui::SliderFloat("Pos X##radar", &Radar::posX, 0.0f, 1600.0f, "%.0f");
            ImGui::SliderFloat("Pos Y##radar", &Radar::posY, 0.0f, 900.0f, "%.0f");
            ImGui::Checkbox("Show Names##radar", &Radar::showNames);
        }
    }

    // ── Movement ──────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Movement"))
    {
        ImGui::Checkbox("BunnyHop", &Movement::bunnyHop);
        ImGui::Checkbox("AutoStrafe", &Movement::autoStrafe);
        if (Movement::autoStrafe)
            ImGui::TextDisabled("  Двигай мышь влево/вправо в прыжке");
    }

    // ── Spectator List ────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Misc", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Spectator List", &SpectatorList::enabled);
        ImGui::Checkbox("Bomb Timer", &BombTimer::enabled);
        ImGui::Checkbox("Hit Marker", &HitMarker::enabled);
        if (HitMarker::enabled)
        {
            ImGui::SliderFloat("Duration##hm", &HitMarker::duration, 0.1f, 1.0f, "%.1fs");
            ImGui::SliderInt("Size##hm", &HitMarker::size, 4, 20);
        }
        ImGui::Separator();
        ImGui::Checkbox("Watermark", &Misc::watermark);
        ImGui::Checkbox("Corner Box", &Misc::cornerBox);
        ImGui::Separator();
        ImGui::Checkbox("Panic Key (F12)", &Misc::panicEnabled);
        ImGui::TextDisabled("  F12 = выкл все / снова = вкл");
    }

    // ── Config ────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Config"))
    {
        static char cfgName[64] = "default";
        ImGui::InputText("Name", cfgName, sizeof(cfgName));
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.2f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.3f, 0.0f, 1.f));
        if (ImGui::Button("Save##cfg", ImVec2(-1, 0))) Config::Save(cfgName);
        ImGui::PopStyleColor(3);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.3f, 0.6f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.4f, 0.8f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.2f, 0.4f, 1.f));
        if (ImGui::Button("Load##cfg", ImVec2(-1, 0))) Config::Load(cfgName);
        ImGui::PopStyleColor(3);
    }

    // ── Unhook ────────────────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, 1.f));
    if (ImGui::Button("Unhook", ImVec2(-1, 0))) MyNamespace::unhook = true;
    ImGui::PopStyleColor(3);

    ImGui::End();
}