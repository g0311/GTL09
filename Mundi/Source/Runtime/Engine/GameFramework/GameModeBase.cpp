#include "pch.h"
#include "GameModeBase.h"
#include "PlayerController.h"
#include "World.h"

IMPLEMENT_CLASS(AGameModeBase)

BEGIN_PROPERTIES(AGameModeBase)
    MARK_AS_SPAWNABLE("게임모드", "게임의 규칙과 기본 설정을 담당하는 액터입니다.")
END_PROPERTIES()

AGameModeBase::AGameModeBase()
{
    Name = "GameModeBase";
    bTickInEditor = false; // 게임 중에만 틱
}

void AGameModeBase::SetWorld(UWorld* InWorld)
{
    // 부모 클래스의 SetWorld 호출
    Super::SetWorld(InWorld);

    // World에 자신을 GameMode로 등록
    if (InWorld)
    {
        InWorld->SetGameMode(this);
        UE_LOG("GameModeBase registered to World");
    }
}

void AGameModeBase::InitGame()
{
    UE_LOG("GameModeBase::InitGame() called");

    // World 가져오기
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG("GameModeBase::InitGame() - World is nullptr");
        return;
    }

    // PlayerController 생성
    APlayerController* PC = World->SpawnPlayerController();
    if (!PC)
    {
        UE_LOG("GameModeBase::InitGame() - Failed to spawn PlayerController");
        return;
    }

    // DefaultPawnActor가 설정되어 있으면 빙의
    if (DefaultPawnActor)
    {
        UE_LOG("GameModeBase::InitGame() - Possessing DefaultPawnActor");
        PC->Possess(DefaultPawnActor);
        UE_LOG("GameModeBase::InitGame() - PlayerController possessed DefaultPawnActor");
    }
    else
    {
        UE_LOG("GameModeBase::InitGame() - No DefaultPawnActor set");
    }
}

void AGameModeBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // TODO: DefaultPawnClass, PlayerControllerClass를 클래스 이름으로 저장/로드
}
