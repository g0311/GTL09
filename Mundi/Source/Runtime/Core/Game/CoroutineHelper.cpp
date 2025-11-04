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
            UE_LOG(("[CoroutineHelper] Coroutine " + std::to_string(State.ID) + " is invalid, removing\n").c_str());

            // Release thread reference
            if (State.ThreadRef != LUA_NOREF && State.MainState)
            {
                luaL_unref(State.MainState, LUA_REGISTRYINDEX, State.ThreadRef);
            }

            it = ActiveCoroutines.erase(it);
            continue;
        }

        // lua_State 상태 확인
        lua_State* L = State.Coroutine.lua_state();
        if (!L)
        {
            UE_LOG(("[CoroutineHelper] Coroutine " + std::to_string(State.ID) + " has null lua_state, removing\n").c_str());

            // Release thread reference
            if (State.ThreadRef != LUA_NOREF && State.MainState)
            {
                luaL_unref(State.MainState, LUA_REGISTRYINDEX, State.ThreadRef);
            }

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
        UE_LOG(("[CoroutineHelper] Executing coroutine " + std::to_string(State.ID) + "...\n").c_str());
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

            // Release thread reference
            if (State.ThreadRef != LUA_NOREF && State.MainState)
            {
                luaL_unref(State.MainState, LUA_REGISTRYINDEX, State.ThreadRef);
            }

            it = ActiveCoroutines.erase(it);
            continue;
        }

        int status = lua_status(L);
        
        // dead = 0 (LUA_OK), yield = LUA_YIELD(1)
        if (status == LUA_OK)
        {
            if (State.CurrentInstruction)
            {
                delete State.CurrentInstruction;
            }

            // Release thread reference
            if (State.ThreadRef != LUA_NOREF && State.MainState)
            {
                luaL_unref(State.MainState, LUA_REGISTRYINDEX, State.ThreadRef);
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
        UE_LOG("[CoroutineHelper] ERROR: EntryPoint is invalid\n");
        return -1;
    }

    if (!OwnerComponent)
    {
        UE_LOG("[CoroutineHelper] ERROR: OwnerComponent is null\n");
        return -1;
    }

    // 1️. OwnerComponent의 스크립트 환경 가져오기
    sol::environment scriptEnv = OwnerComponent->GetScriptEnv();
    if (!scriptEnv.valid())
    {
        UE_LOG("[CoroutineHelper] ERROR: ScriptEnv is invalid\n");
        return -1;
    }

    // 2️. 메인 state 가져오기
    lua_State* MainState = EntryPoint.lua_state();
    UE_LOG(("[CoroutineHelper] MainState: " + std::to_string((uintptr_t)MainState) + "\n").c_str());

    // 3️. 독립적인 Lua 스레드 생성 및 레지스트리에 참조 저장 (GC 방지)
    lua_State* NewThread = lua_newthread(MainState);
    UE_LOG(("[CoroutineHelper] NewThread: " + std::to_string((uintptr_t)NewThread) + "\n").c_str());
    int threadRef = luaL_ref(MainState, LUA_REGISTRYINDEX);  // Thread를 registry에 저장하고 pop
    UE_LOG(("[CoroutineHelper] Thread reference stored in registry: " + std::to_string(threadRef) + "\n").c_str());

    // 4️. EntryPoint 함수를 NewThread에 직접 push
    EntryPoint.push(NewThread);
    UE_LOG(("[CoroutineHelper] EntryPoint pushed to NewThread, top: " + std::to_string(lua_gettop(NewThread)) + "\n").c_str());

    // 5️. CRITICAL: NewThread에서 scriptEnv를 push하고 upvalue 설정
    scriptEnv.push(NewThread);
    UE_LOG(("[CoroutineHelper] scriptEnv pushed to NewThread, top: " + std::to_string(lua_gettop(NewThread)) + "\n").c_str());

    // 6️. NewThread 스택의 함수(-2)의 첫 번째 upvalue를 scriptEnv(-1)로 설정
    const char* upvalueName = lua_setupvalue(NewThread, -2, 1);
    if (upvalueName)
    {
        UE_LOG(("[CoroutineHelper] Set upvalue on NewThread: " + std::string(upvalueName) + "\n").c_str());
    }
    else
    {
        UE_LOG("[CoroutineHelper] WARNING: Failed to set upvalue on NewThread\n");
        lua_pop(NewThread, 1);  // env 제거
    }

    // 7️. 코루틴 생성
    sol::coroutine NewCoroutine(NewThread, -1);

    if (!NewCoroutine.valid())
    {
        UE_LOG("[CoroutineHelper] ERROR: failed to create coroutine\n");
        luaL_unref(MainState, LUA_REGISTRYINDEX, threadRef);  // Clean up thread reference
        return -1;
    }

    int ID = NextCoroutineID++;
    ActiveCoroutines.push_back({ ID, std::move(NewCoroutine), scriptEnv, nullptr, threadRef, MainState });

    UE_LOG(("[CoroutineHelper] Coroutine created with ID: " + std::to_string(ID) + "\n").c_str());
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

			// Release thread reference from registry
			if (it->ThreadRef != LUA_NOREF && it->MainState)
			{
				luaL_unref(it->MainState, LUA_REGISTRYINDEX, it->ThreadRef);
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

		// Release thread reference from registry
		if (State.ThreadRef != LUA_NOREF && State.MainState)
		{
			luaL_unref(State.MainState, LUA_REGISTRYINDEX, State.ThreadRef);
		}
	}
	ActiveCoroutines.clear();
}

FYieldInstruction* FCoroutineHelper::CreateWaitForSeconds(float Seconds)
{
	return new FWaitForSeconds(Seconds);
}
