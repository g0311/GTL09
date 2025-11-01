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
	void StartCoroutine(sol::function EntryPoint);
	FYieldInstruction* WaitForSeconds(float Seconds);
	void StopAllCoroutines();

	// ==================== Lua 이벤트 ====================
	
	/**
	 * @brief 충돌 이벤트를 Lua로 전달
	 * @param OtherActor 충돌한 Actor
	 */
	void NotifyOverlap(AActor* OtherActor);

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

    template<typename ...Args>
    void CallLuaFunction(const FString& InFunctionName, Args&&... InArgs);

private:
	void EnsureCoroutineHelper();

    FString ScriptPath;                 ///< 스크립트 파일 경로
    bool bScriptLoaded = false;   ///< 스크립트 로드 성공 여부
    sol::state* lua = nullptr;          ///< Owning Lua state (per component)

    // Hot-reload on tick
    float HotReloadCheckTimer = 0.0f;        ///< 핫 리로드 체크 타이머
    long long LastScriptWriteTime_ms = 0;    ///< 마지막으로 로드한 스크립트 파일의 수정 시간 (ms)

	FCoroutineHelper* CoroutineHelper{ nullptr };
};

template <typename ... Args>
void UScriptComponent::CallLuaFunction(const FString& InFunctionName, Args&&... InArgs)
{
    if (!bScriptLoaded || !lua)
    {
        return;
    }

    try
    {
        sol::function func = lua[InFunctionName];
        if (func.valid())
        {
            auto result = func(std::forward<Args>(InArgs)...);
            if (!result.valid())
            {
                sol::error err = result;
                UE_LOG("Lua error: %s\n", err.what());
            }
        }
    }
    catch (const sol::error& e)
    {
        UE_LOG("Lua error: %s\n", e.what());
    }
}
