#include "pch.h"
#include "InputMappingContext.h"
#include "InputMappingSubsystem.h"

IMPLEMENT_CLASS(UInputMappingContext)

UInputMappingContext::~UInputMappingContext()
{
    // CRITICAL: 델리게이트 소멸을 완전히 막아야 함
    //
    // 문제: TMap이 소멸되면 내부의 TDelegate 소멸
    //       → std::unordered_map 소멸
    //       → std::function 소멸
    //       → 캡처된 sol::protected_function 소멸
    //       → 이미 해제된 lua_State 접근 → 💥 크래시
    //
    // 해결책: 메모리 릭을 허용하고 소멸을 완전히 막음
    // 방법: TMap을 힙으로 옮기고 delete하지 않음

    // 1. 현재 TMap을 힙으로 옮김 (이동 생성)
    auto* leakedPressed = new TMap<FString, FOnActionEvent>(std::move(ActionPressedDelegates));
    auto* leakedReleased = new TMap<FString, FOnActionEvent>(std::move(ActionReleasedDelegates));
    auto* leakedAxis = new TMap<FString, FOnAxisEvent>(std::move(AxisDelegates));

    // 2. delete하지 않음 - 의도적인 메모리 릭
    // 프로세스 종료 시 OS가 정리
    (void)leakedPressed;
    (void)leakedReleased;
    (void)leakedAxis;

    // 3. 멤버 변수들은 이제 비어있으므로 자동 소멸 시 안전
    // 소멸 시 InputMappingSubsystem에서 즉시 제거 (dangling pointer 방지)
    // 소멸자이므로 Pending이 아닌 즉시 제거 사용
    UInputMappingSubsystem::Get().RemoveMappingContextImmediate(this);
}

void UInputMappingContext::MapAction(const FString& ActionName, int KeyCode, bool bCtrl, bool bAlt, bool bShift, bool bConsume)
{
    FActionKeyMapping M;
    M.ActionName = ActionName;
    M.Source = EInputAxisSource::Key;
    M.KeyCode = KeyCode;
    M.Modifiers = { bCtrl, bAlt, bShift };
    M.bConsume = bConsume;
    ActionMappings.Add(M);
}

void UInputMappingContext::MapActionMouse(const FString& ActionName, EMouseButton Button, bool bConsume)
{
    FActionKeyMapping M;
    M.ActionName = ActionName;
    M.Source = EInputAxisSource::MouseButton;
    M.MouseButton = Button;
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

void UInputMappingContext::ClearAllDelegates()
{
    // sol::function 참조를 해제하기 위해 델리게이트 맵을 명시적으로 비웁니다
    // 이렇게 하면 Lua state가 무효화되기 전에 sol::function 소멸자가 호출됩니다
    ActionPressedDelegates.Empty();
    ActionReleasedDelegates.Empty();
    AxisDelegates.Empty();
}

