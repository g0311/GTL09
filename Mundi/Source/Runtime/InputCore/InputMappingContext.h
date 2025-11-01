#pragma once

#include "Object.h"
#include "UEContainer.h"
#include "Delegate.h"
#include "InputMappingTypes.h"

// Forward
class UInputMappingContext;

// Delegates for actions and axes
DECLARE_DELEGATE_NoParams(FOnActionEvent);
DECLARE_DELEGATE(FOnAxisEvent, float);

class UInputMappingContext : public UObject
{
public:
    DECLARE_CLASS(UInputMappingContext, UObject)

    UInputMappingContext() = default;
    ~UInputMappingContext() override = default;

    // Authoring API
    void MapAction(const FString& ActionName, int KeyCode, bool bCtrl=false, bool bAlt=false, bool bShift=false, bool bConsume=true);
    void MapAxisKey(const FString& AxisName, int KeyCode, float Scale);
    void MapAxisMouse(const FString& AxisName, EInputAxisSource MouseAxis, float Scale=1.0f);

    // Binding API (multiple listeners per name)
    template<typename T>
    FDelegateHandle BindActionPressed(const FString& ActionName, T* Instance, void (T::*Func)())
    {
        return GetActionPressedDelegate(ActionName).AddDynamic(Instance, Func);
    }
    FDelegateHandle BindActionPressed(const FString& ActionName, const FOnActionEvent::HandlerType& Handler)
    {
        return GetActionPressedDelegate(ActionName).Add(Handler);
    }

    template<typename T>
    FDelegateHandle BindActionReleased(const FString& ActionName, T* Instance, void (T::*Func)())
    {
        return GetActionReleasedDelegate(ActionName).AddDynamic(Instance, Func);
    }
    FDelegateHandle BindActionReleased(const FString& ActionName, const FOnActionEvent::HandlerType& Handler)
    {
        return GetActionReleasedDelegate(ActionName).Add(Handler);
    }

    template<typename T>
    FDelegateHandle BindAxis(const FString& AxisName, T* Instance, void (T::*Func)(float))
    {
        return GetAxisDelegate(AxisName).AddDynamic(Instance, Func);
    }
    FDelegateHandle BindAxis(const FString& AxisName, const FOnAxisEvent::HandlerType& Handler)
    {
        return GetAxisDelegate(AxisName).Add(Handler);
    }

    // Internal accessors for subsystem
    const TArray<FActionKeyMapping>& GetActionMappings() const { return ActionMappings; }
    const TArray<FAxisKeyMapping>&  GetAxisMappings() const { return AxisMappings; }

    FOnActionEvent& GetActionPressedDelegate(const FString& Name);
    FOnActionEvent& GetActionReleasedDelegate(const FString& Name);
    FOnAxisEvent&   GetAxisDelegate(const FString& Name);

private:
    TArray<FActionKeyMapping> ActionMappings;
    TArray<FAxisKeyMapping> AxisMappings;

    // Per-name multicast delegates
    TMap<FString, FOnActionEvent> ActionPressedDelegates;
    TMap<FString, FOnActionEvent> ActionReleasedDelegates;
    TMap<FString, FOnAxisEvent> AxisDelegates;
};

