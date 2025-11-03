#pragma once
#include "Actor.h"
#include "Source/Runtime/Core/Delegates/Delegate.h"
#include "sol.hpp"

// Forward Declarations
class APlayerController;
class UScriptComponent;

/**
 * @class AGameModeBase
 * @brief 게임의 규칙, 상태, 승리 조건을 관리하는 Actor
 *
 * GameMode는 World가 소유하며, 게임 전체 로직을 담당합니다.
 * ScriptComponent를 통해 Lua로 게임 로직을 작성할 수 있습니다.
 * PIE 시작 시 PlayerController 생성 및 DefaultPawn 빙의 처리도 담당합니다.
 *
 * 주요 기능:
 * - PlayerController/Pawn 관리
 * - 게임 상태 관리 (점수, 타이머, 게임 오버 등)
 * - Actor 스폰/파괴 관리
 * - 게임 이벤트 브로드캐스트
 * - Lua 스크립팅 지원
 */
class AGameModeBase : public AActor
{
public:
    DECLARE_CLASS(AGameModeBase, AActor)
    GENERATED_REFLECTION_BODY()

    AGameModeBase();
    ~AGameModeBase() override ;

    // ==================== Lifecycle ====================

    /**
     * @brief World 설정 시 자동으로 GameMode 등록
     */
    void SetWorld(UWorld* InWorld) override;

    /**
     * @brief 게임 시작 시 호출 (PIE 시작)
     */
    virtual void InitGame();

    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    void EndPlay(EEndPlayReason Reason) override;

    // ==================== Player Controller / Pawn ====================

    void SetDefaultPawnActor(AActor* InActor) { DefaultPawnActor = InActor; }
    AActor* GetDefaultPawnActor() const { return DefaultPawnActor; }

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
     * @brief Chaser와 플레이어 간의 거리 가져오기
     */
    float GetChaserDistance() const { return ChaserDistance; }

    /**
     * @brief Chaser 거리 설정 (Lua에서 호출)
     */
    void SetChaserDistance(float Distance) { ChaserDistance = Distance; }

    /**
     * @brief 게임 오버 상태 확인
     */
    bool IsGameOver() const { return bIsGameOver; }

    /**
     * @brief 게임 종료
     * @param bVictory true면 승리, false면 패배
     */
    void EndGame(bool bVictory);

    /**
     * @brief 게임 상태를 초기 상태로 리셋 (PIE를 재시작하지 않음)
     * - 점수, 시간, 게임오버 플래그 초기화
     * - 모든 액터를 초기 위치로 복원
     */
    void ResetGame();

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

    // ==================== 동적 이벤트 시스템 ====================

    /**
     * @brief 동적 이벤트 등록 (Lua에서 호출)
     * @param EventName 이벤트 이름 (예: "OnPlayerDeath", "OnItemCollected")
     */
    void RegisterEvent(const FString& EventName);

    /**
     * @brief 이벤트 발행 (Lua에서 호출)
     * @param EventName 이벤트 이름
     * @param EventData Lua 테이블 또는 값 (sol::object). nil이면 파라미터 없이 호출
     */
    void FireEvent(const FString& EventName, sol::object EventData = sol::nil);

    /**
     * @brief 이벤트 구독 (Lua에서 호출)
     * @param EventName 이벤트 이름
     * @param Callback Lua 함수
     * @return 구독 핸들 (구독 해제 시 사용)
     */
    FDelegateHandle SubscribeEvent(const FString& EventName, sol::function Callback);

    /**
     * @brief 이벤트 구독 해제
     * @param EventName 이벤트 이름
     * @param Handle 구독 핸들
     * @return 성공 여부
     */
    bool UnsubscribeEvent(const FString& EventName, FDelegateHandle Handle);

    /**
     * @brief 등록된 모든 이벤트 이름 출력 (디버깅용)
     */
    void PrintRegisteredEvents() const;

    /**
     * @brief 엔진 종료 시 모든 동적 이벤트 델리게이트 정리 (sol::function 참조 해제)
     */
    void ClearAllDynamicEvents();

    // ==================== Serialization ====================
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    void OnSerialized() override;

    DECLARE_DUPLICATE(AGameModeBase)

protected:
    /** PIE 시작 시 PlayerController가 빙의할 레벨의 액터 */
    AActor* DefaultPawnActor = nullptr;

    /* Root Component 설정용 더미 씬 컴포넌트*/
    USceneComponent* GameModeRoot{ nullptr };

    /** ScriptComponent: Lua 로직 */
    UScriptComponent* GameModeScript{ nullptr };

    /** 게임 상태 변수 */
    int32 Score{ 0 };
    float GameTime{ 0.0f };
    float ChaserDistance{ 999.0f };  // Chaser와 플레이어 간 거리
    bool bIsGameOver{ false };
    bool bIsVictory{ false };

    /** 스크립트 경로 (직렬화용) */
    FString ScriptPath;

    /** 동적 이벤트 시스템 */
    // 이벤트 이름 -> (핸들, Lua 콜백) 리스트
    TMap<FString, TArray<std::pair<FDelegateHandle, sol::function>>> DynamicEventMap;
    FDelegateHandle NextDynamicHandle{ 1 };

    /** 지연 삭제 시스템 (Lua 콜백 중 삭제 방지) */
    TArray<AActor*> PendingDestroyActors;

    /** 직렬화 임시 변수 (OnSerialized에서 DefaultPawnActor 복원) */
    FString DefaultPawnActorNameToRestore;
};
