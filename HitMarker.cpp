#include "HitMarker.h"
#include "/test/GUI.h"
#include "output/offsets.hpp"
#include "output/client_dll.hpp"
#include "/test/imgui.h"
#include <Windows.h>
#include <chrono>
#include <vector>
#include <algorithm>

using namespace cs2_dumper::schemas::client_dll;
using namespace cs2_dumper::offsets;

struct HitEvent {
    std::chrono::steady_clock::time_point time;
    float alpha;
};

static std::vector<HitEvent> s_hits;
static int s_lastHits = -1;
static bool s_firstFrame = true;

void DrawHitMarker()
{
    if (!HitMarker::enabled) return;

    auto client = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll"));
    if (!client) return;

    // Чтение LocalPlayerPawn
    uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(client + client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    // Проверка состояния (валидация указателя на здоровье)
    int health = *reinterpret_cast<int*>(localPawn + C_BaseEntity::m_iHealth);
    if (health <= 0) return;

    // Динамическое получение оффсета через cs2_dumper
    // C_CSPlayerPawn::m_totalHitsOnServer
    const int curHits = *reinterpret_cast<int*>(localPawn + CCSPlayer_BulletServices::m_totalHitsOnServer);

    if (s_firstFrame) {
        s_lastHits = curHits;
        s_firstFrame = false;
    }

    if (curHits != s_lastHits && curHits > 0) {
        s_hits.push_back({ std::chrono::steady_clock::now(), 1.0f });
        s_lastHits = curHits;
    }

    if (s_hits.empty()) return;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const float cx = ImGui::GetIO().DisplaySize.x * 0.5f;
    const float cy = ImGui::GetIO().DisplaySize.y * 0.5f;
    const float sz = static_cast<float>(HitMarker::size);
    const float gap = 4.0f;

    auto now = std::chrono::steady_clock::now();

    s_hits.erase(std::remove_if(s_hits.begin(), s_hits.end(),
        [&](const HitEvent& h) {
            return std::chrono::duration<float>(now - h.time).count() >= HitMarker::duration;
        }), s_hits.end());

    for (auto& h : s_hits)
    {
        float elapsed = std::chrono::duration<float>(now - h.time).count();
        float t = 1.0f - (elapsed / HitMarker::duration);
        ImU32 col = IM_COL32(255, 50, 50, static_cast<int>(255 * t));

        dl->AddLine(ImVec2(cx - gap - sz, cy), ImVec2(cx - gap, cy), col, 1.5f);
        dl->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + gap + sz, cy), col, 1.5f);
        dl->AddLine(ImVec2(cx, cy - gap - sz), ImVec2(cx, cy - gap), col, 1.5f);
        dl->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + gap + sz), col, 1.5f);
    }
}