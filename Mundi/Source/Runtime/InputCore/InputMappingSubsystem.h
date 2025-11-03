#pragma once

#include "Object.h"
#include "UEContainer.h"
#include "InputMappingContext.h"
#include "InputManager.h"

// Subsystem that evaluates input mappings every frame
class UInputMappingSubsystem : public UObject
{
public:
    DECLARE_CLASS(UInputMappingSubsystem, UObject)

    UInputMappingSubsystem();
protected:
    ~UInputMappingSubsystem() override;

public:
    static UInputMappingSubsystem& Get();
    static bool IsValid(); // 싱글톤 인스턴스가 유효한지 확인

    void AddMappingContext(UInputMappingContext* Context, int32 Priority);
    void RemoveMappingContext(UInputMappingContext* Context);
    void RemoveMappingContextImmediate(UInputMappingContext* Context); // 소멸자 전용
    void ClearContexts();

    void Tick(float DeltaSeconds);

    // Polling
    bool WasActionPressed(const FString& ActionName) const { return ActionPressed.FindRef(ActionName); }
    bool WasActionReleased(const FString& ActionName) const { return ActionReleased.FindRef(ActionName); }
    bool IsActionDown(const FString& ActionName) const { return ActionDown.FindRef(ActionName); }
    float GetAxisValue(const FString& AxisName) const { return AxisValues.FindRef(AxisName); }

private:
    struct FActiveContext
    {
        UInputMappingContext* Context = nullptr;
        int32 Priority = 0;
    };

    TArray<FActiveContext> ActiveContexts;

    // Pending operations
    struct FPendingOp
    {
        UInputMappingContext* Context = nullptr;
        int32 Priority = 0;
        bool bAdd = true; // true=Add, false=Remove
    };
    TArray<FPendingOp> PendingOps;

    // Frame state for polling
    TMap<FString, bool> ActionPressed;
    TMap<FString, bool> ActionReleased;
    TMap<FString, bool> ActionDown;
    TMap<FString, float> AxisValues;
};

