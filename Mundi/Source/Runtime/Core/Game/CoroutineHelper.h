#pragma once
#include "sol.hpp"

class FYieldInstruction;
class UScriptComponent;


/* *
* @param OwnerComponent - 참조만 할 뿐, 사이클 관리 대상에 포함되지 않으니 소멸 시 nullptr만 선언합니다.
*/
class FCoroutineHelper
{
public:
	FCoroutineHelper(UScriptComponent* InOwner);
	~FCoroutineHelper();

	void RunScheduler(float DeltaTime = 0.f);
	void StartCoroutine(sol::function EntryPoint);
	void Stop();

	// Lua에서 C++ FYieldInstruction 객체를 생성하기 위한 팩토리 함수
	FYieldInstruction* CreateWaitForSeconds(float Seconds);

private:
	void CleanupInstruction();

	UScriptComponent* OwnerComponent{ nullptr };
	sol::coroutine ActiveCoroutine;
	FYieldInstruction* CurrentInstruction{ nullptr }; // 현재 코루틴이 대기 중인 명령
};
