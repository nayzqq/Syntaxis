#include <Windows.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <string>
#include <vector>
#include <sstream>

// Твои оригинальные инклюды
#include "include/MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "GUI.h"
#include "ESP.h"
#include "Aimbot.h"
#include "movement.h"
#include "config.h"
#include "rcs.h"
#include "triggerbot.h"
#include "radar.h"
#include "SpectatorList.h"
#include "BombTimer.h"
#include "HitMarker.h"
#include "Misc.h"

// --- ПОДКЛЮЧАЕМ НАШ НОВЫЙ SDK ---
#include "CS2_SDK/UserCmd.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

static HMODULE              g_hModule = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static IDXGISwapChain* g_pChain = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11RenderTargetView* g_pRTV = nullptr;
static HWND                 g_hwnd = nullptr;
static bool g_inited = false;

// Оригинальные функции D3D11
typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
static Present_t g_oPresent = nullptr;

typedef void(__stdcall* DrawIndexedInstanced_t)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT);
static DrawIndexedInstanced_t g_oDrawII = nullptr;


// ========================================================================
// 1. КОМПАКТНЫЙ СКАНЕР СИГНАТУР (PATTERN SCANNER)
// ========================================================================
uintptr_t FindPattern(const char* module, const char* signature) {
    HMODULE hMod = GetModuleHandleA(module);
    if (!hMod) return 0;

    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hMod;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((uint8_t*)hMod + dosHeader->e_lfanew);
    uint32_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    uint8_t* scanBytes = (uint8_t*)hMod;

    std::vector<int> patternBytes;
    std::stringstream ss(signature);
    std::string word;
    while (ss >> word) {
        if (word == "?" || word == "??") patternBytes.push_back(-1);
        else patternBytes.push_back(std::stoi(word, nullptr, 16));
    }

    size_t s = patternBytes.size();
    int* d = patternBytes.data();

    for (uint32_t i = 0; i < sizeOfImage - s; ++i) {
        bool found = true;
        for (size_t j = 0; j < s; ++j) {
            if (scanBytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) return (uintptr_t)&scanBytes[i];
    }
    return 0;
}

// ========================================================================
// 2. ХУК CREATEMOVE (ТУТ БУДЕТ РАБОТАТЬ ТВОЙ AIM И BHOP)
// ========================================================================
typedef void* (__fastcall* CreateMove_t)(void*, int, uint8_t);
CreateMove_t g_oCreateMove = nullptr;

void* __fastcall hkCreateMove(void* pCSGOInput, int nSlot, uint8_t bActive) {
    // Сначала вызываем оригинал, чтобы игра собрала пакет
    void* result = g_oCreateMove(pCSGOInput, nSlot, bActive);

    if (!pCSGOInput) return result;

    // Получаем наш UserCmd (смещение 0x250 актуально для CS2)
    CUserCmd* pCmd = *reinterpret_cast<CUserCmd**>(reinterpret_cast<uintptr_t>(pCSGOInput) + 0x250);
    if (!pCmd) return result;

    CCSGOInputHistoryEntryPB* pInputEntry = pCmd->GetInputHistoryEntry(0);
    if (!pInputEntry) return result;

    // --- ЗДЕСЬ ПИШЕМ ЛОГИКУ ---
    // Пример: Идеальный Bhop
    // Если сделать проверку флагов игрока из твоих оффсетов, можно удалять прыжок:
    // if (!(localPlayer_Flags & 1)) {
    //     pCmd->nButtons.nValue &= ~IN_JUMP;
    // }

    // Пример: Silent Aim (если есть цель)
    // pInputEntry->pViewAngles->angValue.x = aim_pitch;
    // pInputEntry->pViewAngles->angValue.y = aim_yaw;
    // pInputEntry->pViewAngles->angValue.Clamp();

    return result;
}
// ─────────────────────────────────────────────────────────────────────────────
// Chams: шейдеры и depth state
// ─────────────────────────────────────────────────────────────────────────────
static ID3D11PixelShader* s_Blue = nullptr;
static ID3D11PixelShader* s_Yellow = nullptr;
static ID3D11PixelShader* s_Grey = nullptr;
static ID3D11PixelShader* s_Magenta = nullptr;
static ID3D11PixelShader* s_RedDark = nullptr;
static ID3D11PixelShader* s_Red = nullptr;
static ID3D11PixelShader* s_GreenDark = nullptr;
static ID3D11PixelShader* s_Green = nullptr;
static ID3D11DepthStencilState* s_dsOff = nullptr;

static void MakeShader(ID3D11Device* dev, ID3D11PixelShader** out, float r, float g, float b) {
    if (*out) return;
    char src[256];
    sprintf_s(src, "float4 main():SV_TARGET{return float4(%ff,%ff,%ff,1);}", r, g, b);
    ID3DBlob* blob = nullptr;
    if (SUCCEEDED(D3DCompile(src, strlen(src), 0, 0, 0, "main", "ps_4_0", 0, 0, &blob, 0))) {
        dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out);
        blob->Release();
    }
}

static void InitChams(ID3D11Device* dev) {
    MakeShader(dev, &s_Blue, 0.0f, 0.0f, 1.0f);
    MakeShader(dev, &s_Yellow, 1.0f, 1.0f, 0.0f);
    MakeShader(dev, &s_Grey, 0.3f, 0.3f, 0.3f);
    MakeShader(dev, &s_Magenta, 1.0f, 0.0f, 1.0f);
    MakeShader(dev, &s_RedDark, 0.3f, 0.0f, 0.0f);
    MakeShader(dev, &s_Red, 1.0f, 0.0f, 0.0f);
    MakeShader(dev, &s_GreenDark, 0.0f, 0.3f, 0.0f);
    MakeShader(dev, &s_Green, 0.0f, 1.0f, 0.0f);

    if (!s_dsOff) {
        D3D11_DEPTH_STENCIL_DESC ds{};
        ds.DepthEnable = FALSE;
        ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        ds.DepthFunc = D3D11_COMPARISON_LESS;
        ds.StencilReadMask = 0xFF;
        ds.StencilWriteMask = 0xFF;
        dev->CreateDepthStencilState(&ds, &s_dsOff);
    }
}

static std::string GetDebugName(ID3D11DeviceChild* obj) {
    if (!obj) return "";
    char buf[256]{};
    UINT sz = sizeof(buf);
    obj->GetPrivateData(WKPDID_D3DDebugObjectName, &sz, buf);
    return std::string(buf, sz);
}

// ─────────────────────────────────────────────────────────────────────────────
// Хук DrawIndexedInstanced — чамсы
// ─────────────────────────────────────────────────────────────────────────────
void __stdcall hkDrawIndexedInstanced(
    ID3D11DeviceContext* ctx,
    UINT IndexCount, UINT InstanceCount,
    UINT StartIndex, INT BaseVertex, UINT StartInstance)
{
    auto orig = (DrawIndexedInstanced_t)g_oDrawII;

    // Если оба выключены — просто вызываем оригинал
    if (!MyNamespace::visuals::chams && !MyNamespace::visuals::teamchams) {
        orig(ctx, IndexCount, InstanceCount, StartIndex, BaseVertex, StartInstance);
        return;
    }

    // Проверяем stride (тело=32/другое, голова=28)
    ID3D11Buffer* vb = nullptr; UINT stride = 0, vbOff = 0;
    ctx->IAGetVertexBuffers(0, 1, &vb, &stride, &vbOff);
    if (vb) { vb->Release(); }

    // Проверяем имя шейдера — только персонажи CS2
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    std::string name = GetDebugName(ps);
    if (ps) ps->Release();

    if (name != "csgo_character.vfx_ps") {
        orig(ctx, IndexCount, InstanceCount, StartIndex, BaseVertex, StartInstance);
        return;
    }

    // Команда по IndexCount
    const bool isT = (IndexCount == 3312 || IndexCount == 7218 || IndexCount == 8994 ||
        IndexCount == 14184 || IndexCount == 19437);
    const bool isCT = (IndexCount == 8160 || IndexCount == 8898 ||
        IndexCount == 10242 || IndexCount == 21495);
    const bool isBody = (stride != 28);

    // Сохраняем состояния
    ID3D11PixelShader* origPS = nullptr;
    ID3D11DepthStencilState* origDS = nullptr;
    UINT                     sRef = 0;
    ctx->PSGetShader(&origPS, nullptr, nullptr);
    ctx->OMGetDepthStencilState(&origDS, &sRef);

    // ── Pass 1: depth OFF — рисуем сквозь стены тёмным ───────────────────
    ctx->OMSetDepthStencilState(s_dsOff, sRef);

    if (MyNamespace::visuals::chams)
        ctx->PSSetShader(isBody ? s_Blue : s_Grey, nullptr, 0);

    if (MyNamespace::visuals::teamchams && isBody) {
        if (isT)  ctx->PSSetShader(s_RedDark, nullptr, 0);
        if (isCT) ctx->PSSetShader(s_GreenDark, nullptr, 0);
    }

    orig(ctx, IndexCount, InstanceCount, StartIndex, BaseVertex, StartInstance);

    // ── Pass 2: depth ON — рисуем видимую часть ярким ────────────────────
    ctx->OMSetDepthStencilState(origDS, sRef);

    if (MyNamespace::visuals::chams)
        ctx->PSSetShader(isBody ? s_Yellow : s_Magenta, nullptr, 0);

    if (MyNamespace::visuals::teamchams && isBody) {
        if (isT)  ctx->PSSetShader(s_Red, nullptr, 0);
        if (isCT) ctx->PSSetShader(s_Green, nullptr, 0);
    }

    orig(ctx, IndexCount, InstanceCount, StartIndex, BaseVertex, StartInstance);

    // Восстанавливаем
    ctx->PSSetShader(origPS, nullptr, 0);
    ctx->OMSetDepthStencilState(origDS, sRef);
    if (origPS) origPS->Release();
    if (origDS) origDS->Release();
}

// ─────────────────────────────────────────────────────────────────────────────
// WndProc
// ─────────────────────────────────────────────────────────────────────────────
WNDPROC g_oWndProc = nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT __stdcall hkWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (MyNamespace::menuVisible && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return 0;
    return CallWindowProc(g_oWndProc, hwnd, msg, wp, lp);
}

// ─────────────────────────────────────────────────────────────────────────────
// Хук Present
// ─────────────────────────────────────────────────────────────────────────────
HRESULT __stdcall hkPresent(IDXGISwapChain* pChain, UINT sync, UINT flags) {

    if (!g_inited) {
        pChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice);
        g_pDevice->GetImmediateContext(&g_pContext);

        DXGI_SWAP_CHAIN_DESC sd;
        pChain->GetDesc(&sd);
        g_hwnd = sd.OutputWindow;

        ID3D11Texture2D* buf = nullptr;
        pChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
        g_pDevice->CreateRenderTargetView(buf, nullptr, &g_pRTV);
        buf->Release();

        g_oWndProc = (WNDPROC)SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_pDevice, g_pContext);

        InitChams(g_pDevice);
        Config::Load(); // загружаем конфиг при старте

        g_inited = true;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Переключение меню по HOME
    static bool lastHome = false;
    bool curHome = (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
    if (curHome && !lastHome)
        MyNamespace::menuVisible = !MyNamespace::menuVisible;
    lastHome = curHome;

    if (MyNamespace::menuVisible)
        draw_Menu();

    draw_esp();
    RunMisc();           // panic key — первым
    RunAimbot();
    RunRCS();
    RunTriggerBot();
    RunMovement();
    DrawRadar();
    DrawSpectatorList();
    DrawBombTimer();
    DrawHitMarker();
    DrawWatermark();

    ImGui::EndFrame();
    ImGui::Render();
    g_pContext->OMSetRenderTargets(1, &g_pRTV, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = ((Present_t)g_oPresent)(pChain, sync, flags);

    // Unhook после последнего кадра
    if (MyNamespace::unhook) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_oWndProc);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        FreeLibraryAndExitThread(g_hModule, 0);
    }

    return hr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Инициализация хуков
// ─────────────────────────────────────────────────────────────────────────────
DWORD WINAPI create(void*) {
    // Ждём пока dxgi загрузится
    while (!GetModuleHandleA("dxgi.dll")) Sleep(100);

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetForegroundWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* tmpDev = nullptr;
    ID3D11DeviceContext* tmpCtx = nullptr;
    IDXGISwapChain* tmpChain = nullptr;

    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        levels, 2, D3D11_SDK_VERSION,
        &sd, &tmpChain, &tmpDev, nullptr, &tmpCtx)))
        return 1;

    // Получаем vtable
    void** swapVT = *(void***)tmpChain;   // SwapChain vtable
    void** ctxVT = *(void***)tmpCtx;     // DeviceContext vtable

    void* fnPresent = swapVT[8];   // IDXGISwapChain::Present
    void* fnDrawII = ctxVT[20];   // ID3D11DeviceContext::DrawIndexedInstanced

    tmpDev->Release();
    tmpCtx->Release();
    tmpChain->Release();

    MH_Initialize();

    // 1. Ставим хуки на графику (твои оригинальные)
    MH_CreateHook(fnPresent, hkPresent, (void**)&g_oPresent);
    MH_CreateHook(fnDrawII, hkDrawIndexedInstanced, (void**)&g_oDrawII);

    // =========================================================
    // 2. ИЩЕМ CCSGOInput И СТАВИМ ХУК НА CREATEMOVE (ДОБАВЬ ЭТО)
    // =========================================================
    uintptr_t pCSGOInputInstr = FindPattern("client.dll", "48 8B 0D ? ? ? ? 48 8B 01 FF 50 28");
    if (pCSGOInputInstr) {
        uint32_t offset = *(uint32_t*)(pCSGOInputInstr + 3);
        uintptr_t pCSGOInput = pCSGOInputInstr + offset + 7;

        void* pInputInstance = **(void***)pCSGOInput;
        if (pInputInstance) {
            void** inputVTable = *(void***)pInputInstance;
            void* fnCreateMove = inputVTable[5]; // CreateMove находится по 5-му индексу

            // Создаем хук!
            MH_CreateHook(fnCreateMove, hkCreateMove, (void**)&g_oCreateMove);
        }
    }
    // =========================================================

    // Запускаем ВСЕ созданные хуки разом
    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// DllMain
// ─────────────────────────────────────────────────────────────────────────────
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        Config::Init(hModule);
        CloseHandle(CreateThread(nullptr, 0, create, nullptr, 0, nullptr));
    }
    return TRUE;
}