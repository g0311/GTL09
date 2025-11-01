#pragma once

#include "UEContainer.h"

// Optional modifiers for action key mappings
struct FKeyModifiers
{
    bool bCtrl = false;
    bool bAlt = false;
    bool bShift = false;
};

// Source for axis input
enum class EInputAxisSource : uint8
{
    Key,        // keyboard key treated as axis (1 when down)
    MouseX,     // per-frame delta X
    MouseY,     // per-frame delta Y
    MouseWheel  // per-frame wheel delta
};

// Action mapping (edge-based)
struct FActionKeyMapping
{
    FString ActionName;
    int KeyCode = 0;            // Win32 VK code or ASCII for letters
    FKeyModifiers Modifiers{};  // optional modifier requirements
    bool bConsume = true;       // consume so lower-priority contexts won't see it
};

// Axis mapping (value-based)
struct FAxisKeyMapping
{
    FString AxisName;
    EInputAxisSource Source = EInputAxisSource::Key;
    int KeyCode = 0;      // used when Source == Key
    float Scale = 1.0f;   // contribution scale (also used for mouse axes)
};

