#include "pch.h"
#include "GameModeBase.h"
#include "PlayerController.h"
#include "World.h"
#include "SceneComponent.h"
#include "Source/Runtime/ScriptSys/ScriptComponent.h"
#include "Actor.h"
#include "ObjectFactory.h"

IMPLEMENT_CLASS(AGameModeBase)

BEGIN_PROPERTIES(AGameModeBase)
    MARK_AS_SPAWNABLE("게임모드", "게임의 규칙과 기본 설정을 담당하는 액터입니다.")
    ADD_PROPERTY_SCRIPTPATH(FString, ScriptPath, "Script", true, "게임 모드 Lua 스크립트")
    ADD_PROPERTY(int32, Score, "Game State", true, "현재 점수")
    ADD_PROPERTY(float, GameTime, "Game State", true, "게임 경과 시간 (초)")
    ADD_PROPERTY(bool, bIsGameOver, "Game State", true, "게임 종료 여부")
END_PROPERTIES()

// ==================== Construction ====================
AGameModeBase::AGameModeBase()
{
    Name = "GameModeBase";
    bTickInEditor = false; // 게임 중에만 틱

    // ScriptComponent 생성 및 부착
    GameModeScript = CreateDefaultSubobject<UScriptComponent>("GameModeScript");

    // 기본 테스트 스크립트 설정
    ScriptPath = "gamemode_test_simple.lua";
}

// ==================== Lifecycle ====================
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

void AGameModeBase::BeginPlay()
{
    UE_LOG("[GameModeBase] BeginPlay called\n");
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

    UE_LOG("[GameModeBase] Game Started\n");
}

void AGameModeBase::Tick(float DeltaSeconds)
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

void AGameModeBase::EndPlay(EEndPlayReason Reason)
{
    AActor::EndPlay(Reason);

    UE_LOG("[GameModeBase] Game Ended\n");
}

// ==================== 게임 상태 ====================
void AGameModeBase::SetScore(int32 NewScore)
{
    if (Score != NewScore)
    {
        Score = NewScore;
        OnScoreChangedDelegate.Broadcast(Score);
    }
}

void AGameModeBase::AddScore(int32 Delta)
{
    SetScore(Score + Delta);
}

void AGameModeBase::EndGame(bool bVictory)
{
    if (bIsGameOver)
    {
        return; // 이미 종료됨
    }

    bIsGameOver = true;
    bIsVictory = bVictory;

    UE_LOG(("[GameModeBase] Game Over - " + std::string(bVictory ? "Victory" : "Defeat") + "\n").c_str());

    // 게임 종료 이벤트 발행
    OnGameEndDelegate.Broadcast(bVictory);
}

// ==================== Actor 스폰 ====================
AActor* AGameModeBase::SpawnActorFromLua(const FString& ClassName, const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG("[GameModeBase] Error: No world for spawning actor\n");
        return nullptr;
    }

    // 클래스 찾기
    UClass* Class = UClass::FindClass(FName(ClassName.c_str()));
    if (!Class)
    {
        UE_LOG(("[GameModeBase] Error: Class not found: " + ClassName + "\n").c_str());
        return nullptr;
    }

    // Actor 스폰
    FTransform SpawnTransform;
    SpawnTransform.Translation = Location;

    AActor* NewActor = World->SpawnActor(Class, SpawnTransform);
    if (NewActor)
    {
        UE_LOG(("[GameModeBase] Spawned: " + ClassName + " at (" +
                std::to_string(Location.X) + ", " +
                std::to_string(Location.Y) + ", " +
                std::to_string(Location.Z) + ")\n").c_str());

        // 스폰 이벤트 발행
        OnActorSpawnedDelegate.Broadcast(NewActor);
    }

    return NewActor;
}

bool AGameModeBase::DestroyActorWithEvent(AActor* Actor)
{
    if (!Actor)
    {
        return false;
    }

    // Lua 콜백 중 즉시 삭제하면 크래시하므로 다음 Tick에서 삭제
    if (!PendingDestroyActors.Contains(Actor))
    {
        PendingDestroyActors.Add(Actor);
        UE_LOG(("[GameModeBase] Actor scheduled for destruction: " + Actor->GetName().ToString() + "\n").c_str());
    }

    return true;
}

// ==================== Script Component ====================
void AGameModeBase::SetScriptPath(const FString& Path)
{
    ScriptPath = Path;

    if (GameModeScript)
    {
        GameModeScript->SetScriptPath(Path);
    }
}

// ==================== 동적 이벤트 시스템 ====================
void AGameModeBase::RegisterEvent(const FString& EventName)
{
    if (!DynamicEventMap.Contains(EventName))
    {
        DynamicEventMap.Add(EventName, TArray<std::pair<FDelegateHandle, sol::function>>());
        UE_LOG(("[GameModeBase] Registered dynamic event: " + EventName + "\n").c_str());
    }
}

void AGameModeBase::FireEvent(const FString& EventName, sol::object EventData)
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
                UE_LOG(("[GameModeBase] Event callback error (" + EventName + "): " + FString(e.what()) + "\n").c_str());
            }
        }
    }

    UE_LOG(("[GameModeBase] Fired event: " + EventName + " (" + std::to_string(Callbacks.Num()) + " listeners)\n").c_str());
}

FDelegateHandle AGameModeBase::SubscribeEvent(const FString& EventName, sol::function Callback)
{
    // 이벤트가 없으면 자동 등록
    if (!DynamicEventMap.Contains(EventName))
    {
        RegisterEvent(EventName);
    }

    FDelegateHandle Handle = NextDynamicHandle++;
    DynamicEventMap[EventName].Add({ Handle, Callback });

    UE_LOG(("[GameModeBase] Subscribed to event: " + EventName + " (handle: " + std::to_string(Handle) + ")\n").c_str());
    return Handle;
}

bool AGameModeBase::UnsubscribeEvent(const FString& EventName, FDelegateHandle Handle)
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
            UE_LOG(("[GameModeBase] Unsubscribed from event: " + EventName + " (handle: " + std::to_string(Handle) + ")\n").c_str());
            return true;
        }
    }
    return false;
}

void AGameModeBase::PrintRegisteredEvents() const
{
    UE_LOG("[GameModeBase] ===== Registered Dynamic Events =====\n");
    if (DynamicEventMap.Num() == 0)
    {
        UE_LOG("[GameModeBase] (No events registered)\n");
        return;
    }

    for (const auto& [EventName, Callbacks] : DynamicEventMap)
    {
        UE_LOG(("[GameModeBase] - " + EventName + " (" + std::to_string(Callbacks.Num()) + " listeners)\n").c_str());
    }
    UE_LOG("[GameModeBase] =====================================\n");
}

// ==================== Serialization ====================
void AGameModeBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
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

void AGameModeBase::OnSerialized()
{
    AActor::OnSerialized();

    // 스크립트 재로드
    if (GameModeScript && !ScriptPath.empty())
    {
        GameModeScript->SetScriptPath(ScriptPath);
    }
}
