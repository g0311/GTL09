#include "pch.h"
#include "InputMappingContext.h"
#include "InputMappingSubsystem.h"

IMPLEMENT_CLASS(UInputMappingContext)

UInputMappingContext::~UInputMappingContext()
{
    // 소멸 시 InputMappingSubsystem에서 자동으로 제거 (dangling pointer 방지)
    UInputMappingSubsystem::Get().RemoveMappingContext(this);
}

void UInputMappingContext::MapAction(const FString& ActionName, int KeyCode, bool bCtrl, bool bAlt, bool bShift, bool bConsume)
{
    FActionKeyMapping M;
    M.ActionName = ActionName;
    M.KeyCode = KeyCode;
    M.Modifiers = { bCtrl, bAlt, bShift };
    M.bConsume = bConsume;
    ActionMappings.Add(M);
}

void UInputMappingContext::MapAxisKey(const FString& AxisName, int KeyCode, float Scale)
{
    FAxisKeyMapping M;
    M.AxisName = AxisName;
    M.Source = EInputAxisSource::Key;
    M.KeyCode = KeyCode;
    M.Scale = Scale;
    AxisMappings.Add(M);
}

void UInputMappingContext::MapAxisMouse(const FString& AxisName, EInputAxisSource MouseAxis, float Scale)
{
    FAxisKeyMapping M;
    M.AxisName = AxisName;
    M.Source = MouseAxis;
    M.Scale = Scale;
    AxisMappings.Add(M);
}

FOnActionEvent& UInputMappingContext::GetActionPressedDelegate(const FString& Name)
{
    if (!ActionPressedDelegates.Contains(Name))
    {
        ActionPressedDelegates.Add(Name, FOnActionEvent());
    }
    return *ActionPressedDelegates.Find(Name);
}

FOnActionEvent& UInputMappingContext::GetActionReleasedDelegate(const FString& Name)
{
    if (!ActionReleasedDelegates.Contains(Name))
    {
        ActionReleasedDelegates.Add(Name, FOnActionEvent());
    }
    return *ActionReleasedDelegates.Find(Name);
}

FOnAxisEvent& UInputMappingContext::GetAxisDelegate(const FString& Name)
{
    if (!AxisDelegates.Contains(Name))
    {
        AxisDelegates.Add(Name, FOnAxisEvent());
    }
    return *AxisDelegates.Find(Name);
}

