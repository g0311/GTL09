#include "pch.h"
#include "GameHUDWindow.h"
#include "ImGui/imgui.h"
#include "Source/Slate/Widgets/GameHUDWidget.h"

UGameHUDWindow::UGameHUDWindow()
{
    FUIWindowConfig cfg;
    cfg.WindowTitle = "HUD";
    cfg.DefaultSize = ImVec2(320, 110);
    cfg.DefaultPosition = ImVec2(20, 200);
    cfg.bMovable = false;
    cfg.bResizable = false;
    cfg.bCollapsible = false;
    cfg.UpdateWindowFlags();
    cfg.WindowFlags |= ImGuiWindowFlags_NoSavedSettings;
    SetConfig(cfg);
    SetWindowState(EUIWindowState::Visible);
}

void UGameHUDWindow::Initialize()
{
    // Add a single HUD widget to this window
    AddWidget(new UGameHUDWidget());
}

