#include "Chams.h"
#include "/test/GUI.h"  // MyNamespace::visuals::chams, wallhack, teamchams

#pragma comment(lib, "dxguid.lib") // WKPDID_D3DDebugObjectName

// ─────────────────────────────────────────────────────────────────────────────
// Глобальные шейдеры
// ─────────────────────────────────────────────────────────────────────────────
ID3D11PixelShader* sRed = nullptr;
ID3D11PixelShader* sRedDark = nullptr;
ID3D11PixelShader* sGreen = nullptr;
ID3D11PixelShader* sGreenDark = nullptr;
ID3D11PixelShader* sBlue = nullptr;
ID3D11PixelShader* sYellow = nullptr;
ID3D11PixelShader* sMagenta = nullptr;
ID3D11PixelShader* sGrey = nullptr;

ID3D11DepthStencilState* DepthStencilState_FALSE = nullptr;
ID3D11RasterizerState* CustomState = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные функции
// ─────────────────────────────────────────────────────────────────────────────
std::string GetDebugName(ID3D11DeviceChild* pObject)
{
    if (!pObject) return "";
    char buf[256] = {};
    UINT size = sizeof(buf);
    pObject->GetPrivateData(WKPDID_D3DDebugObjectName, &size, buf);
    return std::string(buf, size);
}

// Генерирует solid-color pixel shader
static void GenerateShader(ID3D11Device* pDevice, ID3D11PixelShader** pShader, float r, float g, float b)
{
    if (*pShader) return; // уже создан

    // HLSL: просто возвращает константный цвет
    char shader_src[256];
    sprintf_s(shader_src,
        "float4 main() : SV_TARGET { return float4(%ff, %ff, %ff, 1.0f); }",
        r, g, b);

    ID3DBlob* pBlob = nullptr;
    ID3DBlob* pErrBlob = nullptr;

    HRESULT hr = D3DCompile(
        shader_src, strlen(shader_src),
        nullptr, nullptr, nullptr,
        "main", "ps_4_0",
        D3DCOMPILE_ENABLE_STRICTNESS, 0,
        &pBlob, &pErrBlob);

    if (FAILED(hr))
    {
        if (pErrBlob) pErrBlob->Release();
        return;
    }

    pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, pShader);
    pBlob->Release();
}

// ─────────────────────────────────────────────────────────────────────────────
// Инициализация — вызывается один раз из hookD3D11Present после получения pDevice
// ─────────────────────────────────────────────────────────────────────────────
void Chams_Init(ID3D11Device* device, ID3D11DeviceContext* /*context*/)
{
    // Шейдеры
    GenerateShader(device, &sRed, 1.0f, 0.0f, 0.0f);
    GenerateShader(device, &sRedDark, 0.2f, 0.0f, 0.0f);
    GenerateShader(device, &sGreen, 0.0f, 1.0f, 0.0f);
    GenerateShader(device, &sGreenDark, 0.0f, 0.2f, 0.0f);
    GenerateShader(device, &sBlue, 0.0f, 0.0f, 1.0f);
    GenerateShader(device, &sYellow, 1.0f, 1.0f, 0.0f);
    GenerateShader(device, &sMagenta, 1.0f, 0.0f, 1.0f);
    GenerateShader(device, &sGrey, 0.3f, 0.3f, 0.3f);

    // Depth stencil — отключённый (для wallhack / видимости сквозь стены)
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;
    device->CreateDepthStencilState(&dsDesc, &DepthStencilState_FALSE);

    // Rasterizer — без culling, для корректного отображения
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthBias = LONG_MIN;
    rsDesc.DepthClipEnable = TRUE;
    rsDesc.AntialiasedLineEnable = FALSE;
    rsDesc.ScissorEnable = FALSE;
    device->CreateRasterizerState(&rsDesc, &CustomState);
}

// ─────────────────────────────────────────────────────────────────────────────
// Основная логика chams — вызывается из хука DrawIndexedInstanced
// ─────────────────────────────────────────────────────────────────────────────
void Chams_Draw(
    ID3D11DeviceContext* pContext,
    UINT IndexCount,
    UINT InstanceCount,
    UINT StartIndexLocation,
    INT  BaseVertexLocation,
    UINT StartInstanceLocation,
    void(__stdcall* originalDraw)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT))
{
    // Читаем vertex buffer чтобы получить Stride
    ID3D11Buffer* veBuffer = nullptr;
    UINT          Stride = 0;
    UINT          veOffset = 0;
    pContext->IAGetVertexBuffers(0, 1, &veBuffer, &Stride, &veOffset);
    if (veBuffer) { veBuffer->Release(); veBuffer = nullptr; }

    // Читаем debug name текущего pixel shader
    ID3D11PixelShader* pPixelShader = nullptr;
    pContext->PSGetShader(&pPixelShader, nullptr, nullptr);
    std::string debugName = GetDebugName(pPixelShader);
    if (pPixelShader) pPixelShader->Release();

    // Только CS2 персонажи
    // Stride == 32 → тело, Stride == 28 → голова
    if (debugName != "csgo_character.vfx_ps")
    {
        originalDraw(pContext, IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        return;
    }

    // Определяем команду по IndexCount
    // T = террористы, CT = контр-террористы
    const bool isT = (IndexCount == 3312 || IndexCount == 7218 ||
        IndexCount == 8994 || IndexCount == 14184 ||
        IndexCount == 19437);
    const bool isCT = (IndexCount == 8160 || IndexCount == 8898 ||
        IndexCount == 10242 || IndexCount == 21495);

    const bool chams = MyNamespace::visuals::chams;
    const bool teamchams = MyNamespace::visuals::teamchams;


    ID3D11PixelShader* oPixelShader = nullptr;
    ID3D11DepthStencilState* origDS = nullptr;
    UINT stencilRef = 0;

    // Сохраняем оригинальные состояния
    pContext->PSGetShader(&oPixelShader, 0, 0);
    pContext->OMGetDepthStencilState(&origDS, &stencilRef);

    // ── Шаг 1: depth OFF — рисуем тёмным сквозь стены ────────────────────
    pContext->OMSetDepthStencilState(DepthStencilState_FALSE, stencilRef);

    if (chams)
        pContext->PSSetShader((Stride != 28) ? sBlue : sGrey, nullptr, 0);
    if (teamchams && Stride != 28)
    {
        if (isT)  pContext->PSSetShader(sRedDark, nullptr, 0);
        if (isCT) pContext->PSSetShader(sGreenDark, nullptr, 0);
    }

    originalDraw(pContext, IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

    // ── Шаг 2: depth ON — рисуем ярким только видимую часть ──────────────
    pContext->OMSetDepthStencilState(origDS, stencilRef);

    if (chams)
        pContext->PSSetShader((Stride != 28) ? sYellow : sMagenta, nullptr, 0);
    if (teamchams && Stride != 28)
    {
        if (isT)  pContext->PSSetShader(sRed, nullptr, 0);
        if (isCT) pContext->PSSetShader(sGreen, nullptr, 0);
    }

    originalDraw(pContext, IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);

    // ── Восстанавливаем оригинальный pixel shader ─────────────────────────
    if (oPixelShader)
    {
        pContext->PSSetShader(oPixelShader, nullptr, 0);
        oPixelShader->Release();
    }

    if (origDS) origDS->Release();
}