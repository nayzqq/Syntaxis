#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <string>

#pragma comment(lib, "D3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// ─────────────────────────────────────────────────────────────────────────────
// Глобальные шейдеры (инициализируются в Chams_Init)
// ─────────────────────────────────────────────────────────────────────────────
extern ID3D11PixelShader* sRed;
extern ID3D11PixelShader* sRedDark;
extern ID3D11PixelShader* sGreen;
extern ID3D11PixelShader* sGreenDark;
extern ID3D11PixelShader* sBlue;
extern ID3D11PixelShader* sYellow;
extern ID3D11PixelShader* sMagenta;
extern ID3D11PixelShader* sGrey;

extern ID3D11DepthStencilState* DepthStencilState_FALSE;
extern ID3D11RasterizerState* CustomState;

// Вызывается один раз после получения pDevice/pContext
void Chams_Init(ID3D11Device* device, ID3D11DeviceContext* context);

// Вызывается внутри hookD3D11DrawIndexedInstanced
void Chams_Draw(
    ID3D11DeviceContext* pContext,
    UINT IndexCount,
    UINT InstanceCount,
    UINT StartIndexLocation,
    INT  BaseVertexLocation,
    UINT StartInstanceLocation,
    // оригинальный указатель на вызов — передаём чтобы не тянуть глобал
    void(__stdcall* originalDraw)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT)
);

// Вспомогательная: читает debug name DX11 объекта
std::string GetDebugName(ID3D11DeviceChild* pObject);