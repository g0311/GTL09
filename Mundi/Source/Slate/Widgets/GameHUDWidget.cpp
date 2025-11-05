#include "pch.h"
#include "GameHUDWidget.h"
#include "ImGui/imgui.h"
#include "Source/Runtime/ScriptSys/ScriptComponent.h"

IMPLEMENT_CLASS(UGameHUDWidget)

void UGameHUDWidget::RenderWidget()
{
    UWorld* World = UUIManager::GetInstance().GetWorld();
    AGameModeBase* GameMode = World ? World->GetGameMode() : nullptr;

    int score = 0;
    float time = 0.0f;
    float chaserDistance = 999.0f;
    bool bGameOver = false;
    if (GameMode)
    {
        score = GameMode->GetScore();
        time = GameMode->GetGameTime();
        chaserDistance = GameMode->GetChaserDistance();
        bGameOver = GameMode->IsGameOver();
    }

    bool drewLua = false;
    if (GameMode)
    {
        if (UScriptComponent* SC = GameMode->GetScriptComponent())
        {
            TArray<UScriptComponent::FHUDRow> rows;
            if (SC->GetHUDEntries(rows))
            {
                drewLua = true;
                for (const auto& r : rows)
                {
                    if (r.bHasColor)
                    {
                        ImGui::TextColored(ImVec4(r.R, r.G, r.B, r.A), "%s: %s", r.Label.c_str(), r.Value.c_str());
                    }
                    else
                    {
                        ImGui::Text("%s: %s", r.Label.c_str(), r.Value.c_str());
                    }
                }
            }
        }
    }
    if (!drewLua)
    {
        // Fallback HUD content inside parent window
        ImGui::Text("Score: %d", score);
        ImGui::Text("Distance to Enemy: %.1f", chaserDistance);
        ImGui::Text("Time: %.1f s", time);
    }

    // Game Over overlay (simple centered window)
    if (bGameOver)
    {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImVec2 size(300, 140);
        ImVec2 pos(vp->WorkPos.x + (vp->WorkSize.x - size.x) * 0.5f,
                   vp->WorkPos.y + (vp->WorkSize.y - size.y) * 0.5f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
        if (ImGui::Begin("GameOverHUD", nullptr, flags))
        {
            bool drewLuaOverlay = false;
            if (GameMode)
            {
                if (UScriptComponent* SC = GameMode->GetScriptComponent())
                {
                    FString title;
                    TArray<FString> lines;
                    if (SC->GetHUDGameOver(title, lines))
                    {
                        drewLuaOverlay = true;
                        ImGui::Text("%s", title.c_str());
                        ImGui::Separator();
                        for (const auto& L : lines)
                        {
                            ImGui::TextUnformatted(L.c_str());
                        }
                    }
                }
            }
            if (!drewLuaOverlay)
            {
                ImGui::Text("Game Over");
                ImGui::Separator();
                ImGui::Text("Final Score: %d", score);
                ImGui::Text("Time: %.1f s", time);
                ImGui::Text("Press R to Restart");

                // R 키 입력 감지 및 게임 리셋
                if (ImGui::IsKeyPressed(ImGuiKey_R) && GameMode)
                {
                    GameMode->ResetGame();
                }
            }
        }
        ImGui::End();
    }
}
