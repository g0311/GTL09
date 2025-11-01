#include "pch.h"
#include "GameMode.h"
#include "SceneComponent.h"
#include "Source/Runtime/ScriptSys/ScriptComponent.h"
#include "World.h"
#include "Actor.h"
#include "ObjectFactory.h"

IMPLEMENT_CLASS(AGameMode)

BEGIN_PROPERTIES(AGameMode)
    ADD_PROPERTY_SCRIPTPATH(FString, ScriptPath, "Script", true, "게임 모드 Lua 스크립트")
    ADD_PROPERTY(int32, Score, "Game State", true, "현재 점수")
    ADD_PROPERTY(float, GameTime, "Game State", true, "게임 경과 시간 (초)")
    ADD_PROPERTY(bool, bIsGameOver, "Game State", true, "게임 종료 여부")
END_PROPERTIES()

// ==================== Construction ====================
AGameMode::AGameMode()
{
    // ScriptComponent 생성 및 부착
    //GameModeRoot= CreateDefaultSubobject<USceneComponent>("Root");
    //if (GameModeRoot)
    //{
    //    GameModeRoot->SetupAttachment(GetRootComponent());
    //}

    GameModeScript = CreateDefaultSubobject<UScriptComponent>("GameModeScript");
}

// ==================== Lifecycle ====================
void AGameMode::BeginPlay()
{
    AActor::BeginPlay();

    // 스크립트 로드
    if (GameModeScript && !ScriptPath.empty())
    {
        GameModeScript->SetScriptPath(ScriptPath);
    }

    // 게임 시작 이벤트 발행
    OnGameStartDelegate.Broadcast();

    UE_LOG("[GameMode] Game Started\n");
}

void AGameMode::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);

    if (!bIsGameOver)
    {
        GameTime += DeltaSeconds;
    }
}

void AGameMode::EndPlay(EEndPlayReason Reason)
{
    AActor::EndPlay(Reason);

    UE_LOG("[GameMode] Game Ended\n");
}

// ==================== 게임 상태 ====================
void AGameMode::SetScore(int32 NewScore)
{
    if (Score != NewScore)
    {
        Score = NewScore;
        OnScoreChangedDelegate.Broadcast(Score);
    }
}

void AGameMode::AddScore(int32 Delta)
{
    SetScore(Score + Delta);
}

void AGameMode::EndGame(bool bVictory)
{
    if (bIsGameOver)
    {
        return; // 이미 종료됨
    }

    bIsGameOver = true;
    bIsVictory = bVictory;

    UE_LOG(("[GameMode] Game Over - " + std::string(bVictory ? "Victory" : "Defeat") + "\n").c_str());

    // 게임 종료 이벤트 발행
    OnGameEndDelegate.Broadcast(bVictory);
}

// ==================== Actor 스폰 ====================
AActor* AGameMode::SpawnActorFromLua(const FString& ClassName, const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG("[GameMode] Error: No world for spawning actor\n");
        return nullptr;
    }

    // 클래스 찾기
    UClass* Class = UClass::FindClass(FName(ClassName.c_str()));
    if (!Class)
    {
        UE_LOG(("[GameMode] Error: Class not found: " + ClassName + "\n").c_str());
        return nullptr;
    }

    // Actor 스폰
    FTransform SpawnTransform;
    SpawnTransform.Translation = Location;

    AActor* NewActor = World->SpawnActor(Class, SpawnTransform);
    if (NewActor)
    {
        UE_LOG(("[GameMode] Spawned: " + ClassName + " at (" +
                std::to_string(Location.X) + ", " +
                std::to_string(Location.Y) + ", " +
                std::to_string(Location.Z) + ")\n").c_str());

        // 스폰 이벤트 발행
        OnActorSpawnedDelegate.Broadcast(NewActor);
    }

    return NewActor;
}

bool AGameMode::DestroyActorWithEvent(AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 파괴 이벤트 발행 (파괴 전에)
    OnActorDestroyedDelegate.Broadcast(Actor);

    return World->DestroyActor(Actor);
}

// ==================== Script Component ====================
void AGameMode::SetScriptPath(const FString& Path)
{
    ScriptPath = Path;

    if (GameModeScript)
    {
        GameModeScript->SetScriptPath(Path);
    }
}

// ==================== Serialization ====================
void AGameMode::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    AActor::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadString(InOutHandle, "ScriptPath", ScriptPath);
        FJsonSerializer::ReadInt32(InOutHandle, "Score", Score);
        FJsonSerializer::ReadFloat(InOutHandle, "GameTime", GameTime);
        FJsonSerializer::ReadBool(InOutHandle, "bIsGameOver", bIsGameOver);
    }
    else
    {
        InOutHandle["ScriptPath"] = ScriptPath.c_str();
        InOutHandle["Score"] = Score;
        InOutHandle["GameTime"] = GameTime;
        InOutHandle["bIsGameOver"] = bIsGameOver;
    }
}

void AGameMode::OnSerialized()
{
    AActor::OnSerialized();

    // 스크립트 재로드
    if (GameModeScript && !ScriptPath.empty())
    {
        GameModeScript->SetScriptPath(ScriptPath);
    }
}
