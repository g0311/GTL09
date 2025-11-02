#include "pch.h"
#include "InputMappingSubsystem.h"
#include "Delegate.h"

IMPLEMENT_CLASS(UInputMappingSubsystem)

static bool TestModifiers(const FKeyModifiers& Mods)
{
    UInputManager& IM = UInputManager::GetInstance();
    if (Mods.bCtrl && !IM.IsKeyDown(VK_CONTROL)) return false;
    if (Mods.bAlt && !IM.IsKeyDown(VK_MENU)) return false;
    if (Mods.bShift && !IM.IsKeyDown(VK_SHIFT)) return false;
    return true;
}

UInputMappingSubsystem& UInputMappingSubsystem::Get()
{
    static UInputMappingSubsystem* Instance = nullptr;
    if (!Instance)
    {
        Instance = NewObject<UInputMappingSubsystem>();
    }
    return *Instance;
}

void UInputMappingSubsystem::AddMappingContext(UInputMappingContext* Context, int32 Priority)
{
    if (!Context) return;
    ActiveContexts.Add({ Context, Priority });
    // Keep highest priority first
    ActiveContexts.Sort([](const FActiveContext& A, const FActiveContext& B) { return A.Priority > B.Priority; });
}

void UInputMappingSubsystem::RemoveMappingContext(UInputMappingContext* Context)
{
    for (int32 i = 0; i < ActiveContexts.Num(); ++i)
    {
        if (ActiveContexts[i].Context == Context)
        {
            ActiveContexts.RemoveAt(i);
            break;
        }
    }
}

void UInputMappingSubsystem::ClearContexts()
{
    ActiveContexts.Empty();
}

void UInputMappingSubsystem::Tick(float /*DeltaSeconds*/)
{
    // Clear transient frame events
    ActionPressed.Empty();
    ActionReleased.Empty();
    AxisValues.Empty();

    UInputManager& IM = UInputManager::GetInstance();

    // Track consumed keys per frame (by VK code)
    TSet<int> ConsumedKeys;

    // 1) Actions: edge-based, respect consumption and priority
    for (const FActiveContext& Ctx : ActiveContexts)
    {
        auto& Maps = Ctx.Context->GetActionMappings();
        for (const FActionKeyMapping& M : Maps)
        {
            // Modifiers must match
            if (!TestModifiers(M.Modifiers))
                continue;

            bool bPressed = false;
            bool bReleased = false;
            bool bDown = false;
            int ConsumeKey = 0;

            if (M.Source == EInputAxisSource::Key)
            {
                const int VK = M.KeyCode;
                ConsumeKey = VK;
                bPressed = IM.IsKeyPressed(VK);
                bReleased = IM.IsKeyReleased(VK);
                bDown = IM.IsKeyDown(VK);
            }
            else if (M.Source == EInputAxisSource::MouseButton)
            {
                // 마우스 버튼은 고유 ID로 consume 처리 (음수로 구분)
                ConsumeKey = -(int)M.MouseButton - 1;
                bPressed = IM.IsMouseButtonPressed(M.MouseButton);
                bReleased = IM.IsMouseButtonReleased(M.MouseButton);
                bDown = IM.IsMouseButtonDown(M.MouseButton);
            }

            // Pressed
            if (!ConsumedKeys.Contains(ConsumeKey) && bPressed)
            {
                ActionPressed.Add(M.ActionName, true);
                ActionDown.Add(M.ActionName, true);
                // Broadcast pressed
                Ctx.Context->GetActionPressedDelegate(M.ActionName).Broadcast();
                if (M.bConsume) ConsumedKeys.Add(ConsumeKey);
            }
            // Released
            if (!ConsumedKeys.Contains(ConsumeKey) && bReleased)
            {
                ActionReleased.Add(M.ActionName, true);
                // Broadcast released
                Ctx.Context->GetActionReleasedDelegate(M.ActionName).Broadcast();
                if (M.bConsume) ConsumedKeys.Add(ConsumeKey);
            }

            // Down state (does not consume)
            if (bDown)
            {
                ActionDown.Add(M.ActionName, true);
            }
        }
    }

    // 2) Axes: value-based, sum across contexts
    float MouseX = IM.GetMouseDelta().X;
    float MouseY = IM.GetMouseDelta().Y;
    float MouseWheel = IM.GetMouseWheelDelta();

    for (const FActiveContext& Ctx : ActiveContexts)
    {
        auto& AxMaps = Ctx.Context->GetAxisMappings();
        for (const FAxisKeyMapping& M : AxMaps)
        {
            float contrib = 0.0f;
            switch (M.Source)
            {
            case EInputAxisSource::Key:
                if (IM.IsKeyDown(M.KeyCode)) contrib = 1.0f * M.Scale;
                break;
            case EInputAxisSource::MouseX:
                contrib = MouseX * M.Scale;
                break;
            case EInputAxisSource::MouseY:
                contrib = MouseY * M.Scale;
                break;
            case EInputAxisSource::MouseWheel:
                contrib = MouseWheel * M.Scale;
                break;
            }

            if (contrib != 0.0f)
            {
                float current = AxisValues.FindRef(M.AxisName);
                AxisValues.Add(M.AxisName, current + contrib);
            }
        }
    }

    // 3) Fire axis delegates for any axes with values (including zero updates if bound?)
    // For simplicity, only fire for non-zero values to avoid spam; polling still works for zero.
    for (const auto& KV : AxisValues)
    {
        const FString& Name = KV.first;
        float Value = KV.second;
        for (const FActiveContext& Ctx : ActiveContexts)
        {
            Ctx.Context->GetAxisDelegate(Name).Broadcast(Value);
        }
    }
}

