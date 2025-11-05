#include "pch.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_2.h>

#include "TextOverlayD2D.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

static inline void SafeRelease(IUnknown* p) { if (p) p->Release(); }

UTextOverlayD2D& UTextOverlayD2D::Get()
{
    static UTextOverlayD2D Instance;
    return Instance;
}

void UTextOverlayD2D::Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext, IDXGISwapChain* InSwapChain)
{
    D3DDevice = InDevice;
    D3DContext = InContext;
    SwapChain = InSwapChain;
    bInitialized = (D3DDevice && D3DContext && SwapChain);
}

void UTextOverlayD2D::Shutdown()
{
    D3DDevice = nullptr;
    D3DContext = nullptr;
    SwapChain = nullptr;
    bInitialized = false;
    TextQueue.clear();
}

static void DrawTextBlock(
    ID2D1DeviceContext* InD2dCtx,
    IDWriteFactory* InDwrite,
    const wchar_t* InText,
    const D2D1_RECT_F& InRect,
    float InFontSize,
    D2D1::ColorF InBgColor,
    D2D1::ColorF InTextColor,
    const wchar_t* InFontFamily,
    const wchar_t* InLocale,
    bool bCenter,
    float CornerRadius)
{
    if (!InD2dCtx || !InDwrite || !InText) return;

    ID2D1SolidColorBrush* BrushFill = nullptr;
    InD2dCtx->CreateSolidColorBrush(InBgColor, &BrushFill);

    ID2D1SolidColorBrush* BrushText = nullptr;
    InD2dCtx->CreateSolidColorBrush(InTextColor, &BrushText);

    if (InBgColor.a > 0.0f)
    {
        if (CornerRadius > 0.0f)
        {
            D2D1_ROUNDED_RECT rr{ InRect, CornerRadius, CornerRadius };
            InD2dCtx->FillRoundedRectangle(&rr, BrushFill);
        }
        else
        {
            InD2dCtx->FillRectangle(InRect, BrushFill);
        }
    }

    const wchar_t* FontFamily = (InFontFamily && wcslen(InFontFamily) > 0) ? InFontFamily : L"Segoe UI";
    const wchar_t* Locale = (InLocale && wcslen(InLocale) > 0) ? InLocale : L"en-us";

    IDWriteTextFormat* Format = nullptr;
    InDwrite->CreateTextFormat(
        FontFamily,
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        InFontSize,
        Locale,
        &Format);

    if (Format)
    {
        Format->SetTextAlignment(bCenter ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING);
        Format->SetParagraphAlignment(bCenter ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        InD2dCtx->DrawTextW(
            InText,
            static_cast<UINT32>(wcslen(InText)),
            Format,
            InRect,
            BrushText);
        Format->Release();
    }

    SafeRelease(BrushText);
    SafeRelease(BrushFill);
}

void UTextOverlayD2D::Draw()
{
    if (!bInitialized || !SwapChain) return;
    if (TextQueue.empty()) return;

    ID2D1Factory1* D2dFactory = nullptr;
    D2D1_FACTORY_OPTIONS Opts{};
#ifdef _DEBUG
    Opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &Opts, (void**)&D2dFactory)))
        return;

    IDXGISurface* Surface = nullptr;
    if (FAILED(SwapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&Surface)))
    {
        SafeRelease(D2dFactory);
        return;
    }

    IDXGIDevice* DxgiDevice = nullptr;
    if (FAILED(D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&DxgiDevice)))
    {
        SafeRelease(Surface);
        SafeRelease(D2dFactory);
        return;
    }

    ID2D1Device* D2dDevice = nullptr;
    if (FAILED(D2dFactory->CreateDevice(DxgiDevice, &D2dDevice)))
    {
        SafeRelease(DxgiDevice);
        SafeRelease(Surface);
        SafeRelease(D2dFactory);
        return;
    }

    ID2D1DeviceContext* D2dCtx = nullptr;
    if (FAILED(D2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2dCtx)))
    {
        SafeRelease(D2dDevice);
        SafeRelease(DxgiDevice);
        SafeRelease(Surface);
        SafeRelease(D2dFactory);
        return;
    }

    IDWriteFactory* Dwrite = nullptr;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&Dwrite)))
    {
        SafeRelease(D2dCtx);
        SafeRelease(D2dDevice);
        SafeRelease(DxgiDevice);
        SafeRelease(Surface);
        SafeRelease(D2dFactory);
        return;
    }

    D2D1_BITMAP_PROPERTIES1 BmpProps = {};
    BmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    BmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    BmpProps.dpiX = 96.0f;
    BmpProps.dpiY = 96.0f;
    BmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

    ID2D1Bitmap1* TargetBmp = nullptr;
    if (FAILED(D2dCtx->CreateBitmapFromDxgiSurface(Surface, &BmpProps, &TargetBmp)))
    {
        SafeRelease(Dwrite);
        SafeRelease(D2dCtx);
        SafeRelease(D2dDevice);
        SafeRelease(DxgiDevice);
        SafeRelease(Surface);
        SafeRelease(D2dFactory);
        return;
    }

    D2dCtx->SetTarget(TargetBmp);
    D2dCtx->BeginDraw();

    // Draw panels behind text (rounded, subtle border)
    if (!PanelQueue.empty())
    {
        for (const auto& p : PanelQueue)
        {
            D2D1_ROUNDED_RECT rr{ D2D1::RectF(p.X, p.Y, p.X + p.Width, p.Y + p.Height), p.CornerRadius, p.CornerRadius };
            ID2D1SolidColorBrush* fill = nullptr; D2dCtx->CreateSolidColorBrush(D2D1::ColorF(p.R, p.G, p.B, p.A), &fill);
            if (fill) { D2dCtx->FillRoundedRectangle(&rr, fill); fill->Release(); }
            if (p.BorderThickness > 0.0f && p.BorderA > 0.0f)
            {
                ID2D1SolidColorBrush* border = nullptr; D2dCtx->CreateSolidColorBrush(D2D1::ColorF(p.BorderR, p.BorderG, p.BorderB, p.BorderA), &border);
                if (border) { D2dCtx->DrawRoundedRectangle(&rr, border, p.BorderThickness); border->Release(); }
            }
        }
        PanelQueue.clear();
    }

    // Flush queued text commands
    for (const auto& cmd : TextQueue)
    {
        D2D1_RECT_F rc = D2D1::RectF(cmd.X, cmd.Y, cmd.X + cmd.Width, cmd.Y + cmd.Height);
        D2D1::ColorF bg(0, 0, 0, cmd.BgAlpha);
        D2D1::ColorF fg(cmd.R, cmd.G, cmd.B, cmd.A);
        const wchar_t* font = cmd.FontFamily.empty() ? L"Malgun Gothic" : cmd.FontFamily.c_str();
        const wchar_t* locale = cmd.Locale.empty() ? L"ko-kr" : cmd.Locale.c_str();
        DrawTextBlock(
            D2dCtx, Dwrite, cmd.Text.c_str(), rc, cmd.Size,
            bg, fg, font, locale, cmd.bCenter, cmd.CornerRadius);
    }
    TextQueue.clear();

    D2dCtx->EndDraw();
    D2dCtx->SetTarget(nullptr);

    SafeRelease(TargetBmp);
    SafeRelease(Dwrite);
    SafeRelease(D2dCtx);
    SafeRelease(D2dDevice);
    SafeRelease(DxgiDevice);
    SafeRelease(Surface);
    SafeRelease(D2dFactory);
}

void UTextOverlayD2D::EnqueueText(const std::wstring& Text,
                                  float X,
                                  float Y,
                                  float FontSize,
                                  float R, float G, float B, float A,
                                  const std::wstring& FontFamily,
                                  const std::wstring& Locale,
                                  float BgAlpha,
                                  float Width,
                                  float Height,
                                  bool bCenter,
                                  float CornerRadius)
{
    FQueuedText cmd;
    cmd.Text = Text;
    cmd.X = X;
    cmd.Y = Y;
    cmd.Size = FontSize;
    cmd.R = R; cmd.G = G; cmd.B = B; cmd.A = A;
    cmd.FontFamily = FontFamily;
    cmd.Locale = Locale;
    cmd.BgAlpha = BgAlpha;
    cmd.Width = Width;
    cmd.Height = Height;
    cmd.bCenter = bCenter;
    cmd.CornerRadius = CornerRadius;
    TextQueue.emplace_back(std::move(cmd));
}

void UTextOverlayD2D::EnqueuePanel(float X,
                                   float Y,
                                   float Width,
                                   float Height,
                                   float R, float G, float B, float A,
                                   float CornerRadius,
                                   float BorderR, float BorderG, float BorderB, float BorderA,
                                   float BorderThickness)
{
    FPanel p;
    p.X = X; p.Y = Y; p.Width = Width; p.Height = Height;
    p.R = R; p.G = G; p.B = B; p.A = A;
    p.CornerRadius = CornerRadius;
    p.BorderR = BorderR; p.BorderG = BorderG; p.BorderB = BorderB; p.BorderA = BorderA;
    p.BorderThickness = BorderThickness;
    PanelQueue.emplace_back(p);
}
