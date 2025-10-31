#pragma once
#include "sol.hpp"

class FYieldInstruction;
class ULuaScriptComponent; // 루아 시스템과 합치지 않아 존재하지 않지만 테스트를 위해 임시 추가

class FCoroutineHelper
{
public:
	FCoroutineHelper(ULuaScriptComponent* InOwner);
	~FCoroutineHelper();

	void RunSceduler(float DeltaTime = 0.f);
	void StartCoroutine(sol::function EntryPoint);

	// Lua에서 C++ YieldInstruction 객체를 생성하기 위한 팩토리 함수
	FYieldInstruction* CreateWaitForSeconds(float Seconds);

private:
	ULuaScriptComponent* OwnerComponent{ nullptr };
	sol::coroutine ActiveCoroutine;
	FYieldInstruction* CurrentInstruction{ nullptr }; // 현재 코루틴이 대기 중인 명령서
};

