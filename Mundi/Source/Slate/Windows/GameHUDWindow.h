#pragma once
#include "Windows/UIWindow.h"

class UGameHUDWindow : public UUIWindow
{
public:
    DECLARE_CLASS(UGameHUDWindow, UUIWindow)

    UGameHUDWindow();
    ~UGameHUDWindow() override = default;

    void Initialize() override;
    bool IsSingleton() override { return true; }
};

