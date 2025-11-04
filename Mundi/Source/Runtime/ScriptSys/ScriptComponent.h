#pragma once
#include "ActorComponent.h"
#include "sol.hpp"
#include "Source/Runtime/Core/Game/YieldInstruction.h"

class FCoroutineHelper;

/**
 * @class UScriptComponent
 * @brief Actor에 Lua 스크립트를 연결하는 컴포넌트
 * 
 * 사용 순서:
 *   1. Actor에 AddComponent<UScriptComponent>()
 *   2. SetScriptPath()로 스크립트 파일 경로 설정
 *   3. Actor의 Tick/BeginPlay 등에서 Lua 함수 호출
 */
class UScriptComponent : public UActorComponent
{
public:
	DECLARE_CLASS(UScriptComponent, UActorComponent)
	GENERATED_REFLECTION_BODY()

	UScriptComponent();
	virtual ~UScriptComponent() override;

	// ==================== Lifecycle ====================
	void BeginPlay() override;
	void TickComponent(float DeltaTime) override;
	void EndPlay(EEndPlayReason Reason) override;

	// ==================== 스크립트 관리 ====================
	
	/**
	 * @brief 스크립트 파일 경로를 설정하고 로드
	 * @param InScriptPath Lua 스크립트 파일 경로
	 * @return 로드 성공 여부
	 */
	bool SetScriptPath(const FString& InScriptPath);
	
	/**
	 * @brief 현재 스크립트 경로 반환
	 */
	const FString& GetScriptPath() const { return ScriptPath; }
	
	/**
	 * @brief 스크립트 파일을 기본 에디터로 열기
	 */
	void OpenScriptInEditor();
	
	/**
	 * @brief 스크립트 재로드 (Hot Reload)
	 */
	bool ReloadScript();

	// ==================== Coroutine ====================
	int StartCoroutine(sol::function EntryPoint);
	void StopCoroutine(int CoroutineID);
	FYieldInstruction* WaitForSeconds(float Seconds);
	void StopAllCoroutines();

	// ==================== Lua 이벤트 ====================
	
	/**
	 * @brief 충돌 이벤트를 Lua로 전달
	 * @param OtherActor 충돌한 Actor
	 */
	void NotifyOverlap(AActor* OtherActor);

	/**
	 * @brief UPrimitiveComponent의 BeginOverlap 델리게이트에서 호출되는 핸들러
	 */
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp);

	/**
	 * @brief UPrimitiveComponent의 EndOverlap 델리게이트에서 호출되는 핸들러 (옵션)
	 */
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp);

	// ==================== Serialize ====================
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	void OnSerialized() override;
	
	// ==================== Duplicate ====================
	void DuplicateSubObjects() override;
	void PostDuplicate() override;
	DECLARE_DUPLICATE(UScriptComponent)
	
	/**
	 * @brief 스크립트가 로드되었는지 확인
	 */
	bool IsScriptLoaded() const { return bScriptLoaded; }

	/**
	 * @brief 스크립트 환경 가져오기 (코루틴에서 사용)
	 */
	sol::environment GetScriptEnv() const { return ScriptEnv; }

    template<typename ...Args>
    void CallLuaFunction(const FString& InFunctionName, Args&&... InArgs);

    // -------- HUD bridge (Lua-driven UI) --------
    struct FHUDRow
    {
        FString Label;
        FString Value;
        bool bHasColor = false;
        float R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f;
    };

    // Calls Lua function HUD_GetEntries() on this component's state.
    // Expects a table array of rows: { {label=..., value=..., color={r,g,b,a}?}, ... }
    bool GetHUDEntries(TArray<FHUDRow>& OutRows);

    // Calls Lua function HUD_GameOver() expecting a table { title=string, lines={...} }
    bool GetHUDGameOver(FString& OutTitle, TArray<FString>& OutLines);

private:
	void EnsureCoroutineHelper();

	void CheckHotReload(float DeltaTime);

    FString ScriptPath;                 ///< 스크립트 파일 경로
    bool bScriptLoaded = false;   ///< 스크립트 로드 성공 여부

    // 전역 Lua state 대신 스크립트별 독립 환경 사용
    sol::environment ScriptEnv;         ///< 스크립트별 독립 환경 (전역 변수 격리)
    sol::table ScriptTable;             ///< 스크립트 함수/변수를 담은 테이블

    // Hot-reload on tick
    float HotReloadCheckTimer = 0.0f;        ///< 핫 리로드 체크 타이머
    long long LastScriptWriteTime_ms = 0;    ///< 마지막으로 로드한 스크립트 파일의 수정 시간 (ms)

	FCoroutineHelper* CoroutineHelper{ nullptr };
};

template <typename ... Args>
void UScriptComponent::CallLuaFunction(const FString& InFunctionName, Args&&... InArgs)
{
	if (!bScriptLoaded || !ScriptTable.valid())
	{
		return;
	}

	try
	{
		sol::protected_function func = ScriptTable[InFunctionName];
		if (func.valid())
		{
			// Set environment for the function call
			sol::environment funcEnv = ScriptEnv;
			sol::set_environment(funcEnv, func);

			auto result = func(std::forward<Args>(InArgs)...);
			if (!result.valid())
			{
				sol::error err = result;

				FString errorMsg = "[Lua Error] " + InFunctionName + ": " + err.what() + "\n";
				UE_LOG(errorMsg.c_str());
			}
		}
	}
	catch (const sol::error& e)
	{
		FString errorMsg = "[Lua Error] Failed to retrieve " + InFunctionName + ": " + e.what() + "\n";
		UE_LOG(errorMsg.c_str());
	}
}
