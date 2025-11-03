#pragma once
#include "Widget.h"
#include "UIManager.h"
#include "Source/Runtime/Engine/GameFramework/GameModeBase.h"

class UGameHUDWidget : public UWidget
{
public:
    DECLARE_CLASS(UGameHUDWidget, UWidget)

    void Initialize() override {}
    void Update() override {}
    void RenderWidget() override;
};

