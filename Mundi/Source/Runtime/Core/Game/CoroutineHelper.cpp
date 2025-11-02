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
	StopAllCoroutines();
	OwnerComponent = nullptr;
}

void FCoroutineHelper::RunScheduler(float DeltaTime)
{
    if (ActiveCoroutines.empty()) { return; }

    for (auto it = ActiveCoroutines.begin(); it != ActiveCoroutines.end();)
    {
        FCoroutineState& State = *it;

        if (!State.Coroutine.valid())
        {
            it = ActiveCoroutines.erase(it);
            continue;
        }

        // YieldInstruction이 남아있으면 대기 처리
        if (State.CurrentInstruction)
        {
            if (!State.CurrentInstruction->IsReady(this, DeltaTime))
            {
                ++it;
                continue;
            }

            delete State.CurrentInstruction;
            State.CurrentInstruction = nullptr;
        }

        // 한 스텝 실행
        sol::protected_function_result Result = State.Coroutine();

        if (!Result.valid())
        {
            sol::error Err = Result;
            FString ErrorMsg = FString("[Coroutine] resume error: ") + Err.what() + "\n";
            UE_LOG(ErrorMsg.c_str());

            if (State.CurrentInstruction)
            {
                delete State.CurrentInstruction;
            }

            it = ActiveCoroutines.erase(it);
            continue;
        }

        lua_State* L = State.Coroutine.lua_state();
        int status = lua_status(L);
        
        // dead = 0 (LUA_OK), yield = LUA_YIELD(1)
        if (status == LUA_OK)
        {
            if (State.CurrentInstruction)
            {
                delete State.CurrentInstruction;
            }

            it = ActiveCoroutines.erase(it);
            continue;
        }

        // yield된 경우, YieldInstruction 처리
        sol::object YieldValue = Result.get<sol::object>();
        if (YieldValue.is<FYieldInstruction*>())
        {
            State.CurrentInstruction = YieldValue.as<FYieldInstruction*>();
        }

        ++it;
    }
}

int FCoroutineHelper::StartCoroutine(sol::function EntryPoint)
{
    if (!EntryPoint.valid())
    {
        return -1;
    }

    // 1️. 독립적인 Lua 스레드 생성 (각 코루틴이 서로 다른 Lua 스택 사용)
    sol::state_view StateView(EntryPoint.lua_state());
    lua_State* NewThread = lua_newthread(StateView.lua_state());

    // 2️. 새 스레드에 함수 푸시 (sol::coroutine은 lua_State* 기반으로 만들 수 있음)
    sol::function ThreadFunction(NewThread, EntryPoint);
    sol::coroutine NewCoroutine = sol::coroutine(NewThread, ThreadFunction);

    if (!NewCoroutine.valid())
    {
        UE_LOG("[Coroutine] failed to create coroutine.\n");
        return -1;
    }

    int ID = NextCoroutineID++;
    ActiveCoroutines.push_back({ ID, std::move(NewCoroutine), nullptr });

    return ID;
}

void FCoroutineHelper::StopCoroutine(int CoroutineID)
{
	for (auto it = ActiveCoroutines.begin(); it != ActiveCoroutines.end(); ++it)
	{
		if (it->ID == CoroutineID)
		{
			if (it->CurrentInstruction)
			{
				delete it->CurrentInstruction;
			}
			ActiveCoroutines.erase(it);
			return;
		}
	}
}

void FCoroutineHelper::StopAllCoroutines()
{
	for (auto& State : ActiveCoroutines)
	{
		if (State.CurrentInstruction)
		{
			delete State.CurrentInstruction;
		}
	}
	ActiveCoroutines.clear();
}

FYieldInstruction* FCoroutineHelper::CreateWaitForSeconds(float Seconds)
{
	return new FWaitForSeconds(Seconds);
}
