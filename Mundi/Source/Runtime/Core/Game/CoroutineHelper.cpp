#include "pch.h"
#include "CoroutineHelper.h"
#include "YieldInstruction.h"

FCoroutineHelper::FCoroutineHelper(ULuaScriptComponent* InOwner)
	: OwnerComponent(InOwner), CurrentInstruction(nullptr)
{
}

FCoroutineHelper::~FCoroutineHelper()
{
	OwnerComponent = nullptr;	// 참조만 할 뿐이므로, nullptr 명시적 선언
	delete CurrentInstruction;
}

void FCoroutineHelper::RunSceduler(float DeltaTime)
{
    if (ActiveCoroutine.valid() == false) { return; }

    bool bShouldResume = false;

    if (CurrentInstruction)
    {
        if (CurrentInstruction->IsReady(this, DeltaTime))
        {
            delete CurrentInstruction;
            CurrentInstruction = nullptr;
            bShouldResume = true;
        }
    }
    else
    {
        // Instruction이 없으면 즉시 재개
        bShouldResume = true;
    }

    auto Result = ActiveCoroutine();

    // 코루틴 종료 확인
    if (Result.get_type() == sol::type::none ||
        ActiveCoroutine.status() == sol::call_status::ok)
    {
        // 코루틴 완료
        ActiveCoroutine = sol::nil;
        if (CurrentInstruction)
        {
            delete CurrentInstruction;
            CurrentInstruction = nullptr;
        }
        return;
    }

    if (Result.get_type() == sol::type::userdata)
    {
        sol::object YieldResult = Result;
        CurrentInstruction = YieldResult.as<FYieldInstruction*>();
    }
}

void FCoroutineHelper::StartCoroutine(sol::function EntryPoint)
{
    if (EntryPoint.valid())
    {
        // 기존 코루틴 정리
        if (CurrentInstruction)
        {
            delete CurrentInstruction;
            CurrentInstruction = nullptr;
        }

        ActiveCoroutine = EntryPoint;
        RunSceduler();
    }
}

FYieldInstruction* FCoroutineHelper::CreateWaitForSeconds(float Seconds)
{
	return new FWaitForSeconds(Seconds);
}
