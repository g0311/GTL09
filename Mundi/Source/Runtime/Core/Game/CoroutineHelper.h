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
	int StartCoroutine(sol::function EntryPoint);
	void StopCoroutine(int CoroutineID);
	void StopAllCoroutines();

	// Lua에서 C++ FYieldInstruction 객체를 생성하기 위한 팩토리 함수
	FYieldInstruction* CreateWaitForSeconds(float Seconds);

private:
	struct FCoroutineState
	{
		int ID;
		sol::coroutine Coroutine;
		FYieldInstruction* CurrentInstruction{ nullptr };
	};

	UScriptComponent* OwnerComponent{ nullptr };
	TArray<FCoroutineState> ActiveCoroutines;
	int NextCoroutineID{ 1 };
};
