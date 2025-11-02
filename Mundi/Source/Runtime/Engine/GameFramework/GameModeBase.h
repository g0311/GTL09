#pragma once
#include "Actor.h"

class APlayerController;

/**
 * @brief 게임의 규칙과 기본 설정을 담당하는 클래스
 * PIE 시작 시 PlayerController 생성 및 DefaultPawn 빙의 처리
 */
class AGameModeBase : public AActor
{
public:
    DECLARE_CLASS(AGameModeBase, AActor)
    GENERATED_REFLECTION_BODY()

    AGameModeBase();
    ~AGameModeBase() override = default;

public:
    /**
     * @brief World 설정 시 자동으로 GameMode 등록
     */
    void SetWorld(UWorld* InWorld) override;

    /**
     * @brief 게임 시작 시 호출 (PIE 시작)
     */
    virtual void InitGame();

    // === Getter/Setter ===
    void SetDefaultPawnActor(AActor* InActor) { DefaultPawnActor = InActor; }
    AActor* GetDefaultPawnActor() const { return DefaultPawnActor; }

    // ───── 직렬화 관련 ────────────────────────────
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    DECLARE_DUPLICATE(AGameModeBase)
protected:
    /** PIE 시작 시 PlayerController가 빙의할 레벨의 액터 */
    AActor* DefaultPawnActor = nullptr;
};
