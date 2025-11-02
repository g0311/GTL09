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

    // 기본 테스트 스크립트 설정
    ScriptPath = "gamemode_simple.lua";
}

// ==================== Lifecycle ====================
void AGameMode::BeginPlay()
{
    UE_LOG("[GameMode] BeginPlay called\n");
    UE_LOG(("  ScriptPath: '" + ScriptPath + "'\n").c_str());
    UE_LOG(("  GameModeScript: " + std::string(GameModeScript ? "valid" : "null") + "\n").c_str());

    // 스크립트 로드 (AActor::BeginPlay() 호출 전에!)
    if (GameModeScript && !ScriptPath.empty())
    {
        UE_LOG("  Setting ScriptPath on GameModeScript...\n");
        bool setResult = GameModeScript->SetScriptPath(ScriptPath);
        UE_LOG(("  SetScriptPath() result: " + std::string(setResult ? "SUCCESS" : "FAILED") + "\n").c_str());
    }
    else
    {
        if (!GameModeScript)
        {
            UE_LOG("  WARNING: GameModeScript is null!\n");
        }
        if (ScriptPath.empty())
        {
            UE_LOG("  WARNING: ScriptPath is empty!\n");
        }
    }

    // 컴포넌트들의 BeginPlay 호출
    AActor::BeginPlay();

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

    // 지연 삭제 처리 (Lua 콜백이 끝난 후 안전하게 삭제)
    if (!PendingDestroyActors.IsEmpty())
    {
        UWorld* World = GetWorld();
        if (World)
        {
            for (AActor* Actor : PendingDestroyActors)
            {
                if (Actor && !Actor->IsPendingDestroy())
                {
                    // 이벤트 발행 (파괴 전에)
                    OnActorDestroyedDelegate.Broadcast(Actor);

                    // 실제 삭제
                    World->DestroyActor(Actor);
                }
            }
        }
        PendingDestroyActors.Empty();
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

    // Lua 콜백 중 즉시 삭제하면 크래시하므로 다음 Tick에서 삭제
    if (!PendingDestroyActors.Contains(Actor))
    {
        PendingDestroyActors.Add(Actor);
        UE_LOG(("[GameMode] Actor scheduled for destruction: " + Actor->GetName().ToString() + "\n").c_str());
    }

    return true;
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

// ==================== 동적 이벤트 시스템 ====================
void AGameMode::RegisterEvent(const FString& EventName)
{
    if (!DynamicEventMap.Contains(EventName))
    {
        DynamicEventMap.Add(EventName, TArray<std::pair<FDelegateHandle, sol::function>>());
        UE_LOG(("[GameMode] Registered dynamic event: " + EventName + "\n").c_str());
    }
}

void AGameMode::FireEvent(const FString& EventName, sol::object EventData)
{
    if (!DynamicEventMap.Contains(EventName))
    {
        // 이벤트가 등록되지 않았으면 무시 (경고 없이)
        return;
    }

    auto& Callbacks = DynamicEventMap[EventName];

    // 모든 구독자에게 이벤트 발행
    for (auto& [Handle, Callback] : Callbacks)
    {
        if (Callback.valid())
        {
            try
            {
                // EventData가 유효하고 nil이 아니면 파라미터로 전달
                if (EventData.valid() && EventData != sol::nil)
                {
                    Callback(EventData);
                }
                else
                {
                    // 파라미터 없이 호출
                    Callback();
                }
            }
            catch (const sol::error& e)
            {
                UE_LOG(("[GameMode] Event callback error (" + EventName + "): " + FString(e.what()) + "\n").c_str());
            }
        }
    }

    UE_LOG(("[GameMode] Fired event: " + EventName + " (" + std::to_string(Callbacks.Num()) + " listeners)\n").c_str());
}

FDelegateHandle AGameMode::SubscribeEvent(const FString& EventName, sol::function Callback)
{
    // 이벤트가 없으면 자동 등록
    if (!DynamicEventMap.Contains(EventName))
    {
        RegisterEvent(EventName);
    }

    FDelegateHandle Handle = NextDynamicHandle++;
    DynamicEventMap[EventName].Add({ Handle, Callback });

    UE_LOG(("[GameMode] Subscribed to event: " + EventName + " (handle: " + std::to_string(Handle) + ")\n").c_str());
    return Handle;
}

bool AGameMode::UnsubscribeEvent(const FString& EventName, FDelegateHandle Handle)
{
    if (!DynamicEventMap.Contains(EventName))
    {
        return false;
    }

    auto& Callbacks = DynamicEventMap[EventName];
    for (int32 i = 0; i < Callbacks.Num(); ++i)
    {
        if (Callbacks[i].first == Handle)
        {
            Callbacks.RemoveAt(i);
            UE_LOG(("[GameMode] Unsubscribed from event: " + EventName + " (handle: " + std::to_string(Handle) + ")\n").c_str());
            return true;
        }
    }
    return false;
}

void AGameMode::PrintRegisteredEvents() const
{
    UE_LOG("[GameMode] ===== Registered Dynamic Events =====\n");
    if (DynamicEventMap.Num() == 0)
    {
        UE_LOG("[GameMode] (No events registered)\n");
        return;
    }

    for (const auto& [EventName, Callbacks] : DynamicEventMap)
    {
        UE_LOG(("[GameMode] - " + EventName + " (" + std::to_string(Callbacks.Num()) + " listeners)\n").c_str());
    }
    UE_LOG("[GameMode] =====================================\n");
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
