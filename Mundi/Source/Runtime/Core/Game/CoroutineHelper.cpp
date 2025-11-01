#include "pch.h"
#include "CoroutineHelper.h"
#include "YieldInstruction.h"
#include "Source/Runtime/ScriptSys/ScriptComponent.h"

FCoroutineHelper::FCoroutineHelper(UScriptComponent* InOwner)
	: OwnerComponent(InOwner)
{
}

FCoroutineHelper::~FCoroutineHelper()
{
	Stop();
	OwnerComponent = nullptr;
}

void FCoroutineHelper::CleanupInstruction()
{
	if (CurrentInstruction)
	{
		delete CurrentInstruction;
		CurrentInstruction = nullptr;
	}
}

void FCoroutineHelper::Stop()
{
	ActiveCoroutine = sol::nil;
	CleanupInstruction();
}

void FCoroutineHelper::RunScheduler(float DeltaTime)
{
	if (!ActiveCoroutine.valid()) { return; }

	if (CurrentInstruction)
	{
		if (const bool bReady = CurrentInstruction->IsReady(this, DeltaTime); bReady == false) 
		{
			return;
		}

		CleanupInstruction();
	}

	sol::protected_function_result Result = ActiveCoroutine();
	if (!Result.valid())
	{
		sol::error Err = Result;
		OutputDebugStringA(("[Coroutine] resume error: " + std::string(Err.what()) + "\n").c_str());
		Stop();
		return;
	}

	sol::object YieldValue = Result.get<sol::object>();

	const sol::call_status Status = ActiveCoroutine.status();
	if (Status == sol::call_status::ok)
	{
		Stop();
		return;
	}

	const sol::type ValueType = YieldValue.get_type();
	if (ValueType == sol::type::userdata)
	{
		CurrentInstruction = YieldValue.as<FYieldInstruction*>();
	}
}

void FCoroutineHelper::StartCoroutine(sol::function EntryPoint)
{
	if (!EntryPoint.valid()) { return; }

	Stop();

	sol::state_view StateView(EntryPoint.lua_state());
	ActiveCoroutine = sol::coroutine(StateView, EntryPoint);
	if (!ActiveCoroutine.valid())
	{
		OutputDebugStringA("[Coroutine] failed to create coroutine.\n");
		return;
	}

	RunScheduler();
}

FYieldInstruction* FCoroutineHelper::CreateWaitForSeconds(float Seconds)
{
	return new FWaitForSeconds(Seconds);
}
