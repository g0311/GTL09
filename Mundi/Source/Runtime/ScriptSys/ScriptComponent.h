#pragma once
#include "ActorComponent.h"

/**
 * @class UScriptComponent
 * @brief Actor에 Lua 스크립트 기능을 부여하는 컴포넌트
 * 
 * 사용법:
 *   1. Actor에 AddComponent<UScriptComponent>()
 *   2. SetScriptPath()로 스크립트 파일 지정
 *   3. Actor의 Tick/BeginPlay 등이 자동으로 Lua 함수 호출
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
     * @brief 스크립트 파일 경로 설정 및 로드
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
     * @brief 스크립트 리로드 (Hot Reload)
     */
    bool ReloadScript();

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
    DECLARE_DUPLICATE(UScriptComponent)
    
    /**
     * @brief 스크립트가 로드되었는지 확인
     */
    bool IsScriptLoaded() const { return bScriptLoaded; }

    template<typename ...Args>
    void CallLuaFunction(const FString& InFunctionName, Args&&... InArgs);

private:
    FString ScriptPath;                 ///< 스크립트 파일 경로
    bool bScriptLoaded = false;   ///< 스크립트 로드 성공 여부
    sol::state* lua;                         ///< 개별 Lua state (컴포넌트별 독립)

    // Hot-reload on tick
    float HotReloadCheckTimer = 0.0f;        ///< 핫 리로드 체크 타이머
    long long LastScriptWriteTime_ms = 0;    ///< 마지막으로 로드한 스크립트 파일의 수정 시간 (ms)
};

template <typename ... Args>
void UScriptComponent::CallLuaFunction(const FString& InFunctionName, Args&&... InArgs)
{
    if (!bScriptLoaded)
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
