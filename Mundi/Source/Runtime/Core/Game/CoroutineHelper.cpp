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
		else
		{
			bShouldResume = true;
		}
	}

	if (bShouldResume)
	{
		auto Result = ActiveCoroutine(); 
		if (Result.get_type() == sol::type::userdata)
		{
			// 코루틴이 새로운 명령서를 발행(yield)했는지 확인합니다.
			sol::object YieldResult = Result;
			CurrentInstruction = YieldResult.as<FYieldInstruction*>();
		}
	}
}

void FCoroutineHelper::StartCoroutine(sol::function EntryPoint)
{
	if (EntryPoint.valid())
	{
		ActiveCoroutine = EntryPoint;
		RunSceduler();
	}
}

FYieldInstruction* FCoroutineHelper::CreateWaitForSeconds(float Seconds)
{
	return new FWaitForSeconds(Seconds);
}
