#pragma once

#include <d3d11.h>
#include <dxgi.h>
#include <string>
#include <vector>

class UTextOverlayD2D
{
public:
    static UTextOverlayD2D& Get();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);
    void Shutdown();
    void Draw();

    void EnqueueText(const std::wstring& Text,
                     float X,
                     float Y,
                     float FontSize,
                     float R, float G, float B, float A,
                     const std::wstring& FontFamily = L"",
                     const std::wstring& Locale = L"",
                     float BgAlpha = 0.0f,
                     float Width = 400.0f,
                     float Height = 40.0f,
                     bool bCenter = false,
                     float CornerRadius = 0.0f);

    // Simple rounded panel background with optional border
    void EnqueuePanel(float X,
                      float Y,
                      float Width,
                      float Height,
                      float R, float G, float B, float A,
                      float CornerRadius = 8.0f,
                      float BorderR = 1.0f, float BorderG = 1.0f, float BorderB = 1.0f, float BorderA = 0.15f,
                      float BorderThickness = 1.0f);

private:
    UTextOverlayD2D() = default;
    ~UTextOverlayD2D() = default;
    UTextOverlayD2D(const UTextOverlayD2D&) = delete;
    UTextOverlayD2D& operator=(const UTextOverlayD2D&) = delete;

    struct FQueuedText
    {
        std::wstring Text;
        std::wstring FontFamily; // empty -> default
        std::wstring Locale;     // empty -> default
        float X = 0.0f;
        float Y = 0.0f;
        float Width = 400.0f;
        float Height = 40.0f;
        float Size = 16.0f;
        float R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f;
        float BgAlpha = 0.0f; // background fill alpha (rounded)
        bool bCenter = false; // center alignment within rect
        float CornerRadius = 0.0f; // rounded background corner radius
    };

    bool bInitialized = false;
    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    std::vector<FQueuedText> TextQueue;

    struct FPanel
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Width = 0.0f;
        float Height = 0.0f;
        float R = 0.0f, G = 0.0f, B = 0.0f, A = 0.0f; // fill
        float CornerRadius = 8.0f;
        float BorderR = 1.0f, BorderG = 1.0f, BorderB = 1.0f, BorderA = 0.0f;
        float BorderThickness = 0.0f;
    };

    std::vector<FPanel> PanelQueue;
};
