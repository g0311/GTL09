#include "pch.h"
#include "GameModeBase.h"
#include "PlayerController.h"
#include "World.h"
#include "SceneComponent.h"
#include "Source/Runtime/ScriptSys/ScriptComponent.h"
#include "Source/Runtime/ScriptSys/UScriptManager.h"
#include "Actor.h"
#include "ObjectFactory.h"
#include "Source/Slate/UIManager.h"
#include "Source/Slate/TextOverlayD2D.h"
#include "Source/Runtime/InputCore/InputManager.h"

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
    //GameModeScript = CreateDefaultSubobject<UScriptComponent>("GameModeScript");

    // ScriptPath를 빈 문자열로 초기화 (Scene에 저장된 기본값 무시)
    ScriptPath = "";
}


AGameModeBase::~AGameModeBase()
{
    // CRITICAL: 델리게이트 소멸을 완전히 막아야 함
    //
    // 문제: TMap이 소멸되면 내부의 sol::function 소멸
    //       → sol::protected_function 소멸자 호출
    //       → 이미 해제된 lua_State 접근 → 💥 크래시
    //
    // 해결책: 메모리 릭을 허용하고 소멸을 완전히 막음
    // 방법: TMap을 힙으로 옮기고 delete하지 않음

    // 1. 현재 TMap을 힙으로 옮김 (이동 생성)
    //auto* leakedEventMap = new TMap<FString, TArray<std::pair<FDelegateHandle, sol::function>>>(std::move(DynamicEventMap));

    // 2. delete하지 않음 - 의도적인 메모리 릭
    // 프로세스 종료 시 OS가 정리
    //(void)leakedEventMap;

    // 3. 멤버 변수는 이제 비어있으므로 자동 소멸 시 안전
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

    // 게임 리셋 이벤트 등록 (스크립트들이 구독할 수 있도록)
    RegisterEvent("OnGameReset");

    // 플레이어 동결 이벤트 등록 (Player 스크립트가 구독하여 이동 멈춤)
    RegisterEvent("FreezePlayer");

    // 플레이어 동결 해제 이벤트 등록 (ResetGame 시 사용)
    RegisterEvent("UnfreezePlayer");

    // Chaser 거리 업데이트 이벤트 등록 (Chaser가 거리 브로드캐스트, Player가 수신)
    RegisterEvent("OnChaserDistanceUpdate");

    // 컴포넌트들의 BeginPlay 호출
    AActor::BeginPlay();

    // 게임 시작 이벤트 발행
    OnGameStartDelegate.Broadcast();

    // No ImGui HUD window. HUD text is now driven by TextOverlayD2D via Lua or engine-side enqueue in Tick().
    {
        (void)UUIManager::GetInstance(); // maintain dependency; no registration
    }

    UE_LOG("[GameModeBase] Game Started\n");
}

void AGameModeBase::Tick(float DeltaSeconds)
{
    AActor::Tick(DeltaSeconds);

    if (!bIsGameOver)
    {
        GameTime += DeltaSeconds;
    }

    // Enqueue a simple HUD using TextOverlayD2D.
    // If Lua provides HUD entries via ScriptComponent, use those; otherwise fallback to engine defaults.
    float x = 20.0f;
    float y = 120.0f;
    const float lineH = 22.0f;
    const float width = 320.0f;
    const float height = 24.0f;

    bool drewLua = false;
    if (UScriptComponent* SC = GetScriptComponent())
    {
        TArray<UScriptComponent::FHUDRow> rows;
        if (SC->GetHUDEntries(rows))
        {
            drewLua = true;
            for (const auto& r : rows)
            {
                std::wstring text = UScriptManager::FStringToWideString(r.Label + ": " + r.Value);
                if (r.bHasColor)
                {
                    UTextOverlayD2D::Get().EnqueueText(text, x, y, 18.0f, r.R, r.G, r.B, r.A, L"", L"", 0.0f, width, height);
                }
                else
                {
                    UTextOverlayD2D::Get().EnqueueText(text, x, y, 18.0f, 1.0f, 1.0f, 1.0f, 1.0f, L"", L"", 0.0f, width, height);
                }
                y += lineH;
            }
        }
    }
    if (!drewLua)
    {
        // Fallback HUD content
        {
            std::wstring w = UScriptManager::FStringToWideString("Score: " + std::to_string(Score));
            UTextOverlayD2D::Get().EnqueueText(w, x, y, 18.0f, 1,1,1,1, L"", L"", 0.0f, width, height); y += lineH;
        }
        {
            std::wstring w = UScriptManager::FStringToWideString("Time: " + std::to_string((int)GameTime) + " s");
            UTextOverlayD2D::Get().EnqueueText(w, x, y, 18.0f, 1,1,1,1, L"", L"", 0.0f, width, height); y += lineH;
        }
        {
            // Distance to Enemy
            char buf[64];
            sprintf_s(buf, "Distance to Enemy: %.1f", GetChaserDistance());
            std::wstring w = UScriptManager::FStringToWideString(buf);
            UTextOverlayD2D::Get().EnqueueText(w, x, y, 18.0f, 1,1,1,1, L"", L"", 0.0f, width, height); y += lineH;
        }
        {
            // Enemy Speed
            char buf[64];
            sprintf_s(buf, "Enemy Speed: %.f", GetChaserSpeed());
            std::wstring w = UScriptManager::FStringToWideString(buf);
            UTextOverlayD2D::Get().EnqueueText(w, x, y, 18.0f, 1,1,1,1, L"", L"", 0.0f, width, height); y += lineH;
        }
        {
            // Player Speed
            char buf[64];
            sprintf_s(buf, "Your Speed: %.f", GetPlayerSpeed());
            std::wstring w = UScriptManager::FStringToWideString(buf);
            UTextOverlayD2D::Get().EnqueueText(w, x, y, 18.0f, 1,1,1,1, L"", L"", 0.0f, width, height); y += lineH;
        }
    }

    // Game Over overlay
    if (IsGameOver())
    {
        // Center the overlay
        FVector2D screen = UInputManager::GetInstance().GetScreenSize();
        float panelW = 420.0f;
        float panelH = 160.0f;
        float px = (screen.X - panelW) * 0.5f;
        float py = (screen.Y - panelH) * 0.5f;
        float cy = py + 10.0f;

        FString title;
        TArray<FString> lines;
        bool hasLuaOverlay = false;
        if (UScriptComponent* SC = GetScriptComponent())
        {
            hasLuaOverlay = SC->GetHUDGameOver(title, lines);
        }

        if (hasLuaOverlay)
        {
            std::wstring wt = UScriptManager::FStringToWideString(title);
            UTextOverlayD2D::Get().EnqueueText(wt, px + 20.0f, cy, 22.0f, 1, 0.7f, 0.7f, 1, L"", L"", 0.0f, panelW - 40.0f, 28.0f);
            cy += 30.0f;
            for (const auto& L : lines)
            {
                std::wstring wl = UScriptManager::FStringToWideString(L);
                UTextOverlayD2D::Get().EnqueueText(wl, px + 20.0f, cy, 18.0f, 1,1,1,1, L"", L"", 0.0f, panelW - 40.0f, 24.0f);
                cy += 22.0f;
            }
        }
        else
        {
            // Default game over content
            UTextOverlayD2D::Get().EnqueueText(L"Game Over", px + 20.0f, cy - 35.0f, 50.0f, 1, 0.5f, 0.5f, 1, L"", L"", 0.0f, panelW - 40.0f, 32.0f);
            cy += 32.0f;

            char l1[64]; sprintf_s(l1, "Final Score: %d", Score);
            std::wstring wl1 = UScriptManager::FStringToWideString(l1);
            UTextOverlayD2D::Get().EnqueueText(wl1, px + 20.0f, cy, 25.0f, 1,1,1,1, L"", L"", 0.0f, panelW - 40.0f, 24.0f);
            cy += 24.0f;

            char l2[64]; sprintf_s(l2, "Time: %.1f s", GameTime);
            std::wstring wl2 = UScriptManager::FStringToWideString(l2);
            UTextOverlayD2D::Get().EnqueueText(wl2, px + 20.0f, cy, 25.0f, 1,1,1,1, L"", L"", 0.0f, panelW - 40.0f, 24.0f);
            cy += 24.0f;

            UTextOverlayD2D::Get().EnqueueText(L"Press R to Restart", px + 20.0f, cy + 4.0f, 25.0f, 1,1,0.5f,1, L"", L"", 0.0f, panelW - 40.0f, 24.0f);
            
            UTextOverlayD2D::Get().EnqueueText(L"팀 3: 정세연 박영빈 정석영 장수빈", px + 20.0f, cy + 40.0f, 25.0f, 1,1,1,1, L"", L"", 0.0f, panelW - 20.0f, 24.0f);
        }

        // Handle restart on R key press
        if (UInputManager::GetInstance().IsKeyPressed('R'))
        {
            ResetGame();
        }
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

    // 게임 종료 이벤트 발행
    OnGameEndDelegate.Broadcast(bVictory);
}

void AGameModeBase::ResetGame()
{
    // 1. 게임 상태 변수 초기화
    Score = 0;
    GameTime = 0.0f;
    ChaserDistance = 999.0f;
    bIsGameOver = false;
    bIsVictory = false;

    // 2. OnGameReset 이벤트 발행 (각 스크립트가 자신의 상태를 초기화)
    FireEvent("OnGameReset", sol::nil);

    // 3. UnfreezePlayer 이벤트 발행 (Player가 다시 움직일 수 있도록)
    FireEvent("UnfreezePlayer", sol::nil);
}

// ==================== Actor 스폰 ====================
AActor* AGameModeBase::SpawnActorFromLua(const FString& ClassName, const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // 클래스 찾기
    UClass* Class = UClass::FindClass(FName(ClassName.c_str()));
    if (!Class)
    {
        return nullptr;
    }

    // Actor 스폰
    FTransform SpawnTransform;
    SpawnTransform.Translation = Location;

    AActor* NewActor = World->SpawnActor(Class, SpawnTransform);
    if (NewActor)
    {
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
    }
}

void AGameModeBase::FireEvent(const FString& EventName, sol::object EventData)
{
    if (!DynamicEventMap.Contains(EventName))
    {
        // 이벤트가 등록되지 않았으면 경고 출력
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
                    // sol::object를 직접 전달하면 메타테이블이 손실될 수 있으므로
                    // AActor*로 변환 시도
                    if (EventData.is<AActor*>())
                    {
                        AActor* actor = EventData.as<AActor*>();
                        Callback(actor);
                    }
                    else
                    {
                        // AActor*가 아니면 그냥 전달
                        Callback(EventData);
                    }
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
          
            return true;
        }
    }
    return false;
}

void AGameModeBase::PrintRegisteredEvents() const
{
    if (DynamicEventMap.Num() == 0)
    {
        return;
    }

    for (const auto& [EventName, Callbacks] : DynamicEventMap)
    {
        UE_LOG(("[GameModeBase] - " + EventName + " (" + std::to_string(Callbacks.Num()) + " listeners)\n").c_str());
    }
}

void AGameModeBase::ClearAllDynamicEvents()
{
    // sol::function 참조를 해제하기 위해 동적 이벤트 맵을 명시적으로 비웁니다
    // 이렇게 하면 Lua state가 무효화되기 전에 sol::function 소멸자가 호출됩니다
    DynamicEventMap.Empty();

    OnGameStartDelegate.Clear();
    OnGameEndDelegate.Clear();
    OnActorSpawnedDelegate.Clear();
    OnActorDestroyedDelegate.Clear();
    OnScoreChangedDelegate.Clear();
}

// ==================== Serialization ====================
void AGameModeBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
    //@TODO UUID를 통해 디폴트 액터 셋하게 변경
    if (bInIsLoading)
    {
        FJsonSerializer::ReadString(InOutHandle, "ScriptPath", ScriptPath);
        FJsonSerializer::ReadInt32(InOutHandle, "Score", Score);
        FJsonSerializer::ReadFloat(InOutHandle, "GameTime", GameTime);
        FJsonSerializer::ReadBool(InOutHandle, "bIsGameOver", bIsGameOver);

        // DefaultPawnActor 이름 로드
        FJsonSerializer::ReadString(InOutHandle, "DefaultPawnActorName", DefaultPawnActorNameToRestore);
    }
    else
    {
        InOutHandle["ScriptPath"] = ScriptPath.c_str();
        InOutHandle["Score"] = Score;
        InOutHandle["GameTime"] = GameTime;
        InOutHandle["bIsGameOver"] = bIsGameOver;

        // DefaultPawnActor 이름 저장
        if (DefaultPawnActor)
        {
            FString PawnName = DefaultPawnActor->GetName().ToString();
            InOutHandle["DefaultPawnActorName"] = PawnName.c_str();
        }
    }
}

void AGameModeBase::OnSerialized()
{
    AActor::OnSerialized();

    // DefaultPawnActor 복원
    if (!DefaultPawnActorNameToRestore.empty())
    {
        UWorld* World = GetWorld();
        if (World)
        {
            // World에서 이름으로 Actor 찾기
            for (AActor* Actor : World->GetActors())
            {
                if (Actor && Actor->GetName().ToString() == DefaultPawnActorNameToRestore)
                {
                    DefaultPawnActor = Actor;
                    break;
                }
            }
        }
        DefaultPawnActorNameToRestore.clear();
    }

    // 스크립트 재로드
    if (GameModeScript && !ScriptPath.empty())
    {
        GameModeScript->SetScriptPath(ScriptPath);
    }
}
