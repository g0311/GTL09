#pragma once
#include "Actor.h"
#include "Source/Runtime/Core/Delegates/Delegate.h"

// Forward Declarations
class UScriptComponent;
class AActor;

/**
 * @class AGameMode
 * @brief 게임의 규칙, 상태, 승리 조건을 관리하는 Actor
 *
 * GameMode는 World가 소유하며, 게임 전체 로직을 담당합니다.
 * ScriptComponent를 통해 Lua로 게임 로직을 작성할 수 있습니다.
 *
 * 주요 기능:
 * - 게임 상태 관리 (점수, 타이머, 게임 오버 등)
 * - Actor 스폰/파괴 관리
 * - 게임 이벤트 브로드캐스트
 * - Lua 스크립팅 지원
 */
class AGameMode : public AActor
{
public:
    DECLARE_CLASS(AGameMode, AActor)
    GENERATED_REFLECTION_BODY()

    AGameMode();
    ~AGameMode() override = default;

    // ==================== Lifecycle ====================
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    void EndPlay(EEndPlayReason Reason) override;

    // ==================== 게임 상태 ====================

    /**
     * @brief 점수 가져오기
     */
    int32 GetScore() const { return Score; }

    /**
     * @brief 점수 설정
     */
    void SetScore(int32 NewScore);

    /**
     * @brief 점수 추가
     */
    void AddScore(int32 Delta);

    /**
     * @brief 게임 시간 가져오기 (초)
     */
    float GetGameTime() const { return GameTime; }

    /**
     * @brief 게임 오버 상태 확인
     */
    bool IsGameOver() const { return bIsGameOver; }

    /**
     * @brief 게임 종료
     * @param bVictory true면 승리, false면 패배
     */
    void EndGame(bool bVictory);

    // ==================== Actor 스폰 ====================

    /**
     * @brief Lua에서 Actor 스폰
     * @param ClassName Actor 클래스 이름 (예: "StaticMeshActor")
     * @param Location 스폰 위치
     * @return 생성된 Actor 포인터
     */
    AActor* SpawnActorFromLua(const FString& ClassName, const FVector& Location);

    /**
     * @brief Actor 파괴 (Delegate 발행 포함)
     */
    bool DestroyActorWithEvent(AActor* Actor);

    // ==================== Script Component ====================

    /**
     * @brief GameMode의 스크립트 컴포넌트 가져오기
     */
    UScriptComponent* GetScriptComponent() const { return GameModeScript; }

    /**
     * @brief 스크립트 경로 설정
     */
    void SetScriptPath(const FString& Path);

    // ==================== 게임 이벤트 Delegates ====================

    // 게임 시작 시 호출
    DECLARE_DELEGATE_NoParams(FOnGameStartSignature);
    FOnGameStartSignature OnGameStartDelegate;

    // 게임 종료 시 호출 (bVictory: 승리 여부)
    DECLARE_DELEGATE(FOnGameEndSignature, bool);
    FOnGameEndSignature OnGameEndDelegate;

    // Actor 스폰 시 호출
    DECLARE_DELEGATE(FOnActorSpawnedSignature, AActor*);
    FOnActorSpawnedSignature OnActorSpawnedDelegate;

    // Actor 파괴 시 호출
    DECLARE_DELEGATE(FOnActorDestroyedSignature, AActor*);
    FOnActorDestroyedSignature OnActorDestroyedDelegate;

    // 점수 변경 시 호출 (NewScore)
    DECLARE_DELEGATE(FOnScoreChangedSignature, int32);
    FOnScoreChangedSignature OnScoreChangedDelegate;

    // Delegate 바인딩 헬퍼
    FDelegateHandle BindOnGameStart(const FOnGameStartSignature::HandlerType& Handler)
    {
        return OnGameStartDelegate.Add(Handler);
    }

    FDelegateHandle BindOnGameEnd(const FOnGameEndSignature::HandlerType& Handler)
    {
        return OnGameEndDelegate.Add(Handler);
    }

    FDelegateHandle BindOnActorSpawned(const FOnActorSpawnedSignature::HandlerType& Handler)
    {
        return OnActorSpawnedDelegate.Add(Handler);
    }

    FDelegateHandle BindOnActorDestroyed(const FOnActorDestroyedSignature::HandlerType& Handler)
    {
        return OnActorDestroyedDelegate.Add(Handler);
    }

    FDelegateHandle BindOnScoreChanged(const FOnScoreChangedSignature::HandlerType& Handler)
    {
        return OnScoreChangedDelegate.Add(Handler);
    }

    // ==================== Serialization ====================
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    void OnSerialized() override;

private:
    /* Root Component 설정용 더미 씬 컴포넌트*/
    USceneComponent* GameModeRoot{ nullptr };

    /** ScriptComponent: Lua 로직 */
    UScriptComponent* GameModeScript{ nullptr };

    /** 게임 상태 변수 */
    int32 Score{ 0 };
    float GameTime{ 0.0f };
    bool bIsGameOver{ false };
    bool bIsVictory{ false };

    /** 스크립트 경로 (직렬화용) */
    FString ScriptPath;
};
