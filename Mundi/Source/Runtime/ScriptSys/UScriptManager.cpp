#include "pch.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "ScriptComponent.h"
#include "ScriptUtils.h"
// GameMode and World
#include "Source/Runtime/Engine/GameplayStatic/GameMode.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
// Collision components
#include "Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Source/Runtime/Engine/Components/ShapeComponent.h"
#include "Source/Runtime/Engine/Components/BoxComponent.h"
// Input
#include "Source/Runtime/InputCore/InputMappingSubsystem.h"
#include "Source/Runtime/InputCore/InputMappingContext.h"
#include "Source/Runtime/InputCore/InputMappingTypes.h"
// Object factory
#include "Source/Runtime/Core/Object/ObjectFactory.h"
// World access
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Source/Runtime/Engine/GameFramework/CameraActor.h"
// Components
#include "PlayerController.h"
#include "RotatingMovementComponent.h"
#include "Source/Runtime/Core/Object/ActorComponent.h"
#include "Source/Runtime/Engine/Components/SceneComponent.h"
#include "Source/Runtime/Engine/Components/StaticMeshComponent.h"
#include "Source/Runtime/Engine/Components/LightComponent.h"
#include "Source/Runtime/Engine/Components/DirectionalLightComponent.h"
#include "Source/Runtime/Engine/Components/CameraComponent.h"
#include "Source/Runtime/Engine/Components/ProjectileMovementComponent.h"
#include "Source/Runtime/Engine/Components/MovementComponent.h"
#include "Source/Runtime/Engine/GameFrameWork/PlayerController.h"

// ==================== 초기화 ====================
IMPLEMENT_CLASS(UScriptManager)

namespace fs = std::filesystem;


bool UScriptManager::CreateScript(const FString& SceneName, const FString& ActorName, FString& OutScriptPath)
{
    // 디버그 로그
    OutputDebugStringA("CreateScript called\n");
    OutputDebugStringA(("  SceneName: '" + SceneName + "'\n").c_str());
    OutputDebugStringA(("  ActorName: '" + ActorName + "'\n").c_str());

    // 한글 확인을 위한 16진수 덤프
    OutputDebugStringA("  ActorName hex: ");
    for (unsigned char C : ActorName)
    {
        char Hex[4];
        sprintf_s(Hex, "%02X ", C);
        OutputDebugStringA(Hex);
    }
    OutputDebugStringA("\n");

    // 입력값 검증
    if (ActorName.empty())
    {
        OutputDebugStringA("Error: ActorName is empty\n");
        return false;
    }

    try
    {
        // 현재 작업 디렉토리 확인
        fs::path CurrentPath = fs::current_path();
        OutputDebugStringA(("Current path: " + CurrentPath.string() + "\n").c_str());

        // 템플릿 경로 (안전한 경로 조합)
        fs::path TemplatePath = CurrentPath / L"Source" / L"Runtime" / L"ScriptSys" / L"template.lua";
        OutputDebugStringA(("Template path: " + TemplatePath.string() + "\n").c_str());

        // ActorName에서 파일명으로 사용할 수 없는 문자만 제거 (한글은 유지)
        FString SanitizedActorName;
        const std::string InvalidChars = "<>:\"/\\|?*";
        for (char C : ActorName)
        {
            // 유효하지 않은 문자가 아니면 포함
            if (InvalidChars.find(C) == std::string::npos)
            {
                SanitizedActorName += C;
            }
        }

        if (SanitizedActorName.empty())
        {
            SanitizedActorName = "Actor";
        }

        // 새 스크립트 파일명 생성
        // SceneName이 비어있으면 ActorName만 사용, 아니면 SceneName_ActorName 형식
        FString ScriptName;
        if (SceneName.empty())
        {
            ScriptName = SanitizedActorName;
            // .lua 확장자가 없으면 추가
            if (ScriptName.find(".lua") == FString::npos)
            {
                ScriptName += ".lua";
            }
        }
        else
        {
            ScriptName = SceneName + "_" + SanitizedActorName + ".lua";
        }
        OutputDebugStringA(("Script name: " + ScriptName + "\n").c_str());

        // LuaScripts 디렉토리 경로 (안전한 경로 조합)
        fs::path LuaScriptsDir = CurrentPath / L"LuaScripts";

        // LuaScripts 디렉토리가 없으면 생성
        if (!fs::exists(LuaScriptsDir))
        {
            fs::create_directories(LuaScriptsDir);
            OutputDebugStringA("Created LuaScripts directory\n");
        }

        std::wstring ScriptNameW = FStringToWideString(ScriptName);
        if (ScriptNameW.empty() && !ScriptName.empty())
        {
            OutputDebugStringA("Failed to convert script name to wide string\n");
            return false;
        }

        OutputDebugStringA("Script name (wide): ");
        OutputDebugStringW((ScriptNameW + L"\n").c_str());

        fs::path newScriptPath = LuaScriptsDir / ScriptNameW;
        OutputDebugStringA("New script path created\n");

        // 1. 템플릿 파일 존재 확인
        if (!fs::exists(TemplatePath))
        {
            OutputDebugStringA(("Error: template.lua not found at: " + TemplatePath.string() + "\n").c_str());
            return false;
        }

        // 2. 이미 파일이 존재하는 경우 성공 반환 (중복 생성 방지)
        if (fs::exists(newScriptPath))
        {
            OutputDebugStringA(("Script already exists: " + ScriptName + "\n").c_str());
            OutScriptPath = ScriptName;
            return true;
        }

        // 3. 템플릿 파일을 새 이름으로 복사
        fs::copy_file(TemplatePath, newScriptPath);
        OutputDebugStringA(("Created script: " + newScriptPath.string() + "\n").c_str());

        // 생성된 파일명 반환
        OutScriptPath = ScriptName;
        return true;
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(("Exception in CreateScript: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

void UScriptManager::EditScript(const FString& ScriptPath)
{
    // 1. 경로 해석
    fs::path absolutePath = UScriptManager::ResolveScriptPath(ScriptPath);

    // 2. 파일 존재 확인
    if (!fs::exists(absolutePath))
    {
        MessageBoxA(NULL, "Script file not found", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // 3. Windows 기본 에디터로 열기 (wide string 버전 사용)
    HINSTANCE hInst = ShellExecuteW(NULL, L"open", absolutePath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    if ((INT_PTR)hInst <= 32)
    {
        MessageBoxA(NULL, "Failed to open script file", "Error", MB_OK | MB_ICONERROR);
    }
}

void UScriptManager::InitializeGlobalLuaState()
{
    if (GlobalLuaState)
    {
        UE_LOG("[ScriptManager] Global Lua state already initialized\n");
        return;
    }

    UE_LOG("[ScriptManager] Initializing global Lua state...\n");
    GlobalLuaState = std::make_unique<sol::state>();
    GlobalLuaState->open_libraries(
        sol::lib::base,
        sol::lib::package,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::os,
        sol::lib::math,
        sol::lib::table,
        sol::lib::debug,
        sol::lib::bit32,
        sol::lib::io,
        sol::lib::utf8
    );
    // 모든 타입 등록
    RegisterTypesToState(GlobalLuaState.get());

    UE_LOG("[ScriptManager] Global Lua state initialized successfully\n");
}

void UScriptManager::RegisterTypesToState(sol::state* state)
{
    if (!state) return;

    RegisterCoreTypes(state);

    // World 등록
    RegisterWorld(state);

    // Actor 먼저 등록
    RegisterActor(state);

    // 컴포넌트 등록 (상속 순서대로)
    RegisterActorComponent(state);
    RegisterSceneComponent(state);
    RegisterStaticMeshComponent(state);
    RegisterLightComponent(state);
    RegisterDirectionalLightComponent(state);
    RegisterCameraComponent(state);
    RegisterScriptComponent(state);
    RegisterPlayerController(state);
    RegisterGameMode(state);

    // Collision components
    RegisterPrimitiveComponent(state);
    RegisterShapeComponent(state);
    RegisterProjectileMovement(state);
    RegisterRotatingMovement(state);
    RegisterBoxComponent(state);

    // Input
    RegisterInputEnums(state);
    RegisterInputContext(state);
    RegisterInputSubsystem(state);
}

// ==================== 스크립트 파일 관리 ====================
/**
 * @brief 상대 스크립트 경로를 절대 경로(std::filesystem::path)로 변환
 */
std::filesystem::path UScriptManager::ResolveScriptPath(const FString& RelativePath)
{
    std::wstring PathW = FStringToWideString(RelativePath);
    if (PathW.empty() && !RelativePath.empty())
    {
        OutputDebugStringA("Failed to convert path to wide string\n");
        return fs::path();
    }

    fs::path ScriptPathFs(PathW);

    if (ScriptPathFs.is_absolute())
    {
        OutputDebugStringA("Path is absolute\n");
        return ScriptPathFs;
    }
    else
    {
        // 기본 경로: current_path (Mundi) / LuaScripts
        fs::path Resolved = fs::current_path() / L"LuaScripts" / PathW;
        OutputDebugStringA("Resolved to: ");
        OutputDebugStringW((Resolved.wstring() + L"\n").c_str());
        return Resolved;
    }
}

std::wstring UScriptManager::FStringToWideString(const FString& UTF8Str)
{
    // Case A. 파일명이 비어 있으면 즉시 종료합니다.
    if (UTF8Str.empty()) { return L""; }

    // Case B. 정상적으로 변환하지 못하면 즉시 종료합니다.
    int WideSize = MultiByteToWideChar(CP_UTF8, 0, UTF8Str.c_str(), -1, NULL, 0);
    if (WideSize <= 0)
    {
        OutputDebugStringA("Failed to convert path to wide string\n");
        return L"";
    }

    // 1. FString(UTF-8) 문자열을 wstring(UTF-16) 변환합니다.
    std::wstring Path(WideSize - 1, 0); // null 종결 문자 제외
    MultiByteToWideChar(CP_UTF8, 0, UTF8Str.c_str(), -1, &Path[0], WideSize);

    return Path;
}

void UScriptManager::RegisterCoreTypes(sol::state* state)
{
    RegisterLOG(state);
    RegisterFName(state);
    RegisterVector(state);
    RegisterQuat(state);
    RegisterTransform(state);
}

void UScriptManager::RegisterLOG(sol::state* state)
{
    // ==================== Log 등록 ====================
    state->open_libraries();
    state->set_function("Log", [](const std::string& msg) {
        UE_LOG(("[Lua] " + msg + "\n").c_str());
    });
    state->set_function("LogWarning", [](const std::string& msg) {
        UE_LOG(("[Lua Warning] " + msg + "\n").c_str());
    });
    state->set_function("LogError", [](const std::string& msg) {
        UE_LOG(("[Lua Error] " + msg + "\n").c_str());
    });
}

void UScriptManager::RegisterFName(sol::state* state)
{
    // ==================== FName 등록 ====================
    BEGIN_LUA_TYPE(state, FName, "FName",
        []() { return FName(); },
        [](const char* s) { return FName(s); }
    )
        ADD_LUA_FUNCTION("ToString", &FName::ToString)
        // 문자열 연결을 위한 메타함수
        ADD_LUA_META_FUNCTION(to_string, &FName::ToString)
    END_LUA_TYPE()
}

void UScriptManager::RegisterVector(sol::state* state)
{
    // ==================== FVector 등록 ====================
    BEGIN_LUA_TYPE(state, FVector, "Vector",
        []() { return FVector(); },
        [](float x, float y, float z) { return FVector(x, y, z); }
    )
        ADD_LUA_PROPERTY("X", &FVector::X)
        ADD_LUA_PROPERTY("Y", &FVector::Y)
        ADD_LUA_PROPERTY("Z", &FVector::Z)
        ADD_LUA_META_FUNCTION(addition, [](const FVector& a, const FVector& b) {
            return a + b;
        })
        ADD_LUA_META_FUNCTION(subtraction, [](const FVector& a, const FVector& b) {
            return a - b;
        })
        ADD_LUA_META_FUNCTION(multiplication, [](const FVector& v, float f) -> FVector {
            return v * f;
        })
        ADD_LUA_META_FUNCTION(division, [](const FVector& v, float f) {
            return v / f;
        })
    END_LUA_TYPE() // 4. 종료
}

void UScriptManager::RegisterQuat(sol::state* state)
{
    // ==================== FQuat 등록 ====================
    BEGIN_LUA_TYPE(state, FQuat, "Quat",
        []() { return FQuat(); },
        [](float x, float y, float z, float w) { return FQuat(x, y, z, w); }
    )
        ADD_LUA_PROPERTY("X", &FQuat::X)
        ADD_LUA_PROPERTY("Y", &FQuat::Y)
        ADD_LUA_PROPERTY("Z", &FQuat::Z)
        ADD_LUA_PROPERTY("W", &FQuat::W)
    END_LUA_TYPE()
}

void UScriptManager::RegisterTransform(sol::state* state)
{
    // ==================== FTransform 등록 ====================
    BEGIN_LUA_TYPE(state, FTransform, "Transform",
        []() { return FTransform(); },
        [](const FVector& t, const FQuat& r, const FVector& s) { return FTransform(t, r, s); }
    )
        ADD_LUA_PROPERTY("Location", &FTransform::Translation)
        ADD_LUA_PROPERTY("Rotation", &FTransform::Rotation)
        ADD_LUA_PROPERTY("Scale", &FTransform::Scale3D)
    END_LUA_TYPE()
}

void UScriptManager::RegisterWorld(sol::state* state)
{
    // ==================== UWorld 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UWorld, "World")
        // Actor Spawning
        ADD_LUA_FUNCTION("SpawnActorByClass", [](UWorld* world, const std::string& className) -> AActor* {
        if (!world) return nullptr;

        // UClass를 통해 클래스 이름으로 액터 생성
        UClass* ActorClass = UClass::FindClass(FName(className.c_str()));
        if (!ActorClass)
        {
            UE_LOG(("Failed to find class: " + className + "\n").c_str());
            return nullptr;
        }

        return world->SpawnActor(ActorClass);
            })

        ADD_LUA_FUNCTION("SpawnActorByClassWithTransform", [](UWorld* world, const std::string& className, const FTransform& transform) -> AActor* {
        if (!world) return nullptr;

        UClass* ActorClass = UClass::FindClass(FName(className.c_str()));
        if (!ActorClass)
        {
            UE_LOG(("Failed to find class: " + className + "\n").c_str());
            return nullptr;
        }

        return world->SpawnActor(ActorClass, transform);
            })

        // Actor Destruction
        ADD_LUA_FUNCTION("DestroyActor", &UWorld::DestroyActor)

        // Actor Queries
        ADD_LUA_FUNCTION("GetActors", [](UWorld* world, sol::this_state s) -> sol::table {
        sol::state_view lua(s);
        sol::table actorsTable = lua.create_table();

        if (!world) return actorsTable;

        const TArray<AActor*>& actors = world->GetActors();
        for (int i = 0; i < actors.Num(); ++i)
        {
            actorsTable[i + 1] = actors[i];  // Lua는 1-based 인덱싱
        }

        return actorsTable;
            })

        // Camera
        ADD_LUA_FUNCTION("GetCameraActor", &UWorld::GetCameraActor)
        ADD_LUA_FUNCTION("SetCameraActor", &UWorld::SetCameraActor)

        // ==================== UWorld 등록 ====================
        ADD_LUA_FUNCTION("GetGameMode", &UWorld::GetGameMode)
        ADD_LUA_OVERLOAD("SpawnActor",
            static_cast<AActor * (UWorld::*)(UClass*)>(&UWorld::SpawnActor),
            static_cast<AActor * (UWorld::*)(UClass*, const FTransform&)>(&UWorld::SpawnActor)
        )
        ADD_LUA_FUNCTION("DestroyActor", &UWorld::DestroyActor)

        // ==================== Time Dilation API (Hit Stop / Slow Motion) ====================
        ADD_LUA_FUNCTION("StartHitStop", sol::overload(
            // 기본값: 0.01 dilation (1% 속도)
            [](UWorld* world, float duration) {
                if (world) world->StartHitStop(duration);
            },
            // 커스텀 dilation 지정
            [](UWorld* world, float duration, float dilation) {
                if (world) world->StartHitStop(duration, dilation);
            }
        ))
        ADD_LUA_FUNCTION("IsHitStopActive", &UWorld::IsHitStopActive)
        ADD_LUA_FUNCTION("SetGlobalTimeDilation", &UWorld::SetGlobalTimeDilation)
        ADD_LUA_FUNCTION("GetGlobalTimeDilation", &UWorld::GetGlobalTimeDilation)
        ADD_LUA_FUNCTION("StartSlowMotion", &UWorld::StartSlowMotion)
        ADD_LUA_FUNCTION("StopSlowMotion", &UWorld::StopSlowMotion)
        ADD_LUA_FUNCTION("GetRealDeltaTime", &UWorld::GetRealDeltaTime)
        END_LUA_TYPE()

        // Global accessor GetGameMode()
        state->set_function("GetGameMode", [](sol::this_state L, sol::this_environment env) -> AGameModeBase* {
        // Actor의 World를 통해 GameMode 접근
        // environment에서 actor를 찾음 (독립 environment 지원)
        sol::environment current_env = env;
        sol::optional<AActor*> actorOpt = current_env["actor"];

        // environment에서 못 찾으면 전역 state에서 찾기
        if (!actorOpt)
        {
            sol::state_view lua(L);
            actorOpt = lua["actor"];
        }

        if (!actorOpt)
        {
            UE_LOG("[Lua] Error: GetGameMode() - 'actor' not found in environment or global state\n");
            return nullptr;
        }

        AActor* actor = actorOpt.value();
        if (!actor)
        {
            UE_LOG("[Lua] Error: GetGameMode() - actor is null\n");
            return nullptr;
        }

        UWorld* world = actor->GetWorld();
        if (!world)
        {
            UE_LOG("[Lua] Error: GetGameMode() - actor has no world\n");
            return nullptr;
        }

        AGameModeBase* gameMode = world->GetGameMode();
        if (!gameMode)
        {
            UE_LOG("[Lua] Error: GetGameMode() - world has no GameMode\n");
            return nullptr;
        }
        return gameMode;
        });

    // Reset game state (PIE를 재시작하지 않고 게임 상태만 초기화)
    state->set_function("ResetGame", []() {
        UE_LOG("[Lua] ResetGame() called - resetting game state...\n");

        UWorld* World = GEngine.GetDefaultWorld();
        if (!World)
        {
            UE_LOG("[Lua] Error: ResetGame() - No World\n");
            return;
        }

        AGameModeBase* GameMode = World->GetGameMode();
        if (!GameMode)
        {
            UE_LOG("[Lua] Error: ResetGame() - No GameMode\n");
            return;
        }

        GameMode->ResetGame();
        UE_LOG("[Lua] ResetGame() completed successfully\n");
        });

    // Get PlayerPawn (현재 플레이어가 조종하는 Pawn)
    state->set_function("GetPlayerPawn", [](sol::this_state L, sol::this_environment env) -> AActor* {
        // environment에서 actor를 찾음 (독립 environment 지원)
        sol::environment current_env = env;
        sol::optional<AActor*> actorOpt = current_env["actor"];

        // environment에서 못 찾으면 전역 state에서 찾기
        if (!actorOpt)
        {
            sol::state_view lua(L);
            actorOpt = lua["actor"];
        }

        if (!actorOpt)
        {
            UE_LOG("[Lua] Error: GetPlayerPawn() - 'actor' not found in environment or global state");
            return nullptr;
        }

        AActor* actor = actorOpt.value();
        if (!actor)
        {
            UE_LOG("[Lua] Error: GetPlayerPawn() - actor is null");
            return nullptr;
        }

        UWorld* world = actor->GetWorld();
        if (!world)
        {
            UE_LOG("[Lua] Error: GetPlayerPawn() - actor has no world");
            return nullptr;
        }

        APlayerController* pc = world->GetPlayerController();
        if (!pc)
        {
            UE_LOG("[Lua] Error: GetPlayerPawn() - world has no PlayerController");
            return nullptr;
        }

        AActor* pawn = pc->GetPawn();
        if (!pawn)
        {
            UE_LOG("[Lua] Warning: GetPlayerPawn() - PlayerController has no possessed pawn");
            return nullptr;
        }

        return pawn;
        });

    // Global actor search functions
        state->set_function("FindActorByName", [](sol::this_state L, sol::this_environment env, const FString& name) -> AActor* {
            // environment에서 actor를 찾음 (독립 environment 지원)
            sol::environment current_env = env;
            sol::optional<AActor*> actorOpt = current_env["actor"];

            // environment에서 못 찾으면 전역 state에서 찾기
            if (!actorOpt)
            {
                sol::state_view lua(L);
                actorOpt = lua["actor"];
            }
            if (!actorOpt) return nullptr;

            AActor* actor = actorOpt.value();
            UWorld* world = actor ? actor->GetWorld() : nullptr;
            if (!world || !world->GetLevel()) return nullptr;

            for (AActor* a : world->GetLevel()->GetActors())
            {
                if (a && a->GetName().ToString() == name)
                {
                    return a;
                }
            }
            return nullptr; }
        );

        state->set_function("FindActorsByClass", [](sol::this_state L, sol::this_environment env, const FString& className) -> sol::table {
            sol::state_view lua(L);
            sol::table result = lua.create_table();

            // environment에서 actor를 찾음 (독립 environment 지원)
            sol::environment current_env = env;
            sol::optional<AActor*> actorOpt = current_env["actor"];

            // environment에서 못 찾으면 전역 state에서 찾기
            if (!actorOpt)
            {
                actorOpt = lua["actor"];
            }
            if (!actorOpt) return result;

            AActor* actor = actorOpt.value();
            UWorld* world = actor ? actor->GetWorld() : nullptr;
            if (!world || !world->GetLevel()) return result;

            int idx = 1;
            for (AActor* a : world->GetLevel()->GetActors())
            {
                if (a && FName(a->GetClass()->Name) == FName(className.c_str()))
                {
                    result[idx++] = a;
                }
            }
            return result;
            });

        // Global SpawnActor helper (위치만 지정)
        state->set_function("SpawnActor", [](sol::this_state L, sol::this_environment env, const FString& className, const FVector& location) -> AActor* {
            // environment에서 actor를 찾음 (독립 environment 지원)
            sol::environment current_env = env;
            sol::optional<AActor*> actorOpt = current_env["actor"];

            // environment에서 못 찾으면 전역 state에서 찾기
            if (!actorOpt)
            {
                sol::state_view lua(L);
                actorOpt = lua["actor"];
            }
            if (!actorOpt) return nullptr;

            AActor* actor = actorOpt.value();
            UWorld* world = actor ? actor->GetWorld() : nullptr;
            if (!world) return nullptr;

            AGameModeBase* gameMode = world->GetGameMode();
            if (!gameMode) return nullptr;

            return gameMode->SpawnActorFromLua(className, location);
            });
}

void UScriptManager::RegisterActor(sol::state* state)
{
    // ==================== AActor 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, AActor, "Actor")
        // Transform API
        ADD_LUA_FUNCTION("GetActorLocation", &AActor::GetActorLocation)
        ADD_LUA_FUNCTION("SetActorLocation", &AActor::SetActorLocation)
        ADD_LUA_FUNCTION("GetActorRotation", &AActor::GetActorRotation)
        ADD_LUA_OVERLOAD("SetActorRotation",
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::SetActorRotation),
            static_cast<void(AActor::*)(const FVector&)>(&AActor::SetActorRotation)
        )
        ADD_LUA_FUNCTION("GetActorScale", &AActor::GetActorScale)
        ADD_LUA_FUNCTION("SetActorScale", &AActor::SetActorScale)

        // Movement API
        ADD_LUA_FUNCTION("AddActorWorldLocation", &AActor::AddActorWorldLocation)
        ADD_LUA_FUNCTION("AddActorLocalLocation", &AActor::AddActorLocalLocation)
        ADD_LUA_OVERLOAD("AddActorWorldRotation",
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorWorldRotation),
            static_cast<void(AActor::*)(const FVector&)>(&AActor::AddActorWorldRotation)
        )
        ADD_LUA_OVERLOAD("AddActorLocalRotation",
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorLocalRotation),
            static_cast<void(AActor::*)(const FVector&)>(&AActor::AddActorLocalRotation)
        )

        ADD_LUA_FUNCTION("GetActorForward", &AActor::GetActorForward)
        ADD_LUA_FUNCTION("GetActorRight", &AActor::GetActorRight)
        ADD_LUA_FUNCTION("GetActorUp", &AActor::GetActorUp)

        // Name/Visibility
        ADD_LUA_FUNCTION("GetName", [](AActor* actor) -> std::string {
            return actor->GetName().ToString();
        })
        ADD_LUA_FUNCTION("SetActorHiddenInGame", &AActor::SetActorHiddenInGame)
        ADD_LUA_FUNCTION("GetActorHiddenInGame", &AActor::GetActorHiddenInGame)

        // Lifecycle
        ADD_LUA_FUNCTION("Destroy", &AActor::Destroy)

        // Component access helpers
        ADD_LUA_FUNCTION("GetShapeComponent", [](AActor* actor) -> UShapeComponent* {
            return actor ? actor->GetComponent<UShapeComponent>() : nullptr;
        })
        ADD_LUA_FUNCTION("GetPrimitiveComponent", [](AActor* actor) -> UPrimitiveComponent* {
            return actor ? actor->GetComponent<UPrimitiveComponent>() : nullptr;
        })
        ADD_LUA_FUNCTION("GetCameraComponent", [](AActor* actor) -> UCameraComponent*
            {
                if (!actor) return nullptr;
                for (UActorComponent* Comp : actor->GetOwnedComponents())
                {
                    if (auto* C = Cast<UCameraComponent>(Comp))
                        return C;
                }
                return nullptr;
            })

        ADD_LUA_FUNCTION("GetStaticMeshComponent", [](AActor* actor) -> UStaticMeshComponent*
            {
                if (!actor) return nullptr;
                for (UActorComponent* Comp : actor->GetOwnedComponents())
                {
                    if (auto* M = Cast<UStaticMeshComponent>(Comp))
                        return M;
                }
                return nullptr;
            })

        // Light component accessor (Directional/Point/Spot share ULightComponent interface)
        ADD_LUA_FUNCTION("GetLightComponent", [](AActor* actor) -> ULightComponent*
            {
                if (!actor) return nullptr;
                // Check root first (many light actors keep the light as root)
                if (USceneComponent* Root = actor->GetRootComponent())
                {
                    if (auto* L = Cast<ULightComponent>(Root))
                        return L;
                }
                for (UActorComponent* Comp : actor->GetOwnedComponents())
                {
                    if (auto* L = Cast<ULightComponent>(Comp))
                        return L;
                }
                return nullptr;
            })

        // Name/Visibility
        ADD_LUA_FUNCTION("GetName", [](AActor* actor) -> std::string {
        return actor->GetName().ToString();
            })
        ADD_LUA_FUNCTION("SetActorHiddenInGame", &AActor::SetActorHiddenInGame)
        ADD_LUA_FUNCTION("GetActorHiddenInGame", &AActor::GetActorHiddenInGame)

        // World Access
        ADD_LUA_FUNCTION("GetWorld", &AActor::GetWorld)

        // Component Access
        ADD_LUA_FUNCTION("GetOwnedComponents", [](AActor* actor, sol::this_state s) -> sol::table {
        // TSet을 Lua 테이블로 변환
        sol::state_view lua(s);
        sol::table components = lua.create_table();
        int index = 1;
        for (UActorComponent* comp : actor->GetOwnedComponents()) {
            components[index++] = comp;
        }
        return components;
            })
        ADD_LUA_FUNCTION("GetRootComponent", &AActor::GetRootComponent)

        // Lifecycle
        ADD_LUA_FUNCTION("Destroy", &AActor::Destroy)

        // 스크립트 컴포넌트 추가 (런타임 동적 생성)
        ADD_LUA_FUNCTION("AddScriptComponent", [](AActor* actor) -> UScriptComponent* {
            if (!actor) return nullptr;

            // 런타임에 동적으로 컴포넌트 생성
            UScriptComponent* comp = ObjectFactory::NewObject<UScriptComponent>();
            if (!comp) return nullptr;

            // Actor에 컴포넌트 추가
            actor->AddOwnedComponent(comp);

            // World가 있으면 컴포넌트 등록 및 초기화
            if (UWorld* world = actor->GetWorld())
            {
                comp->RegisterComponent(world);
                comp->InitializeComponent();

                // BeginPlay는 SetScriptPath 후에 호출되도록 여기서는 호출하지 않음
                // Lua에서 SetScriptPath 후 수동으로 BeginPlay를 호출해야 함
            }

            return comp;
        })
    END_LUA_TYPE()
}

void UScriptManager::RegisterActorComponent(sol::state* state)
{
    // ==================== UActorComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UActorComponent, "ActorComponent")
        ADD_LUA_FUNCTION("GetOwner", &UActorComponent::GetOwner)
        ADD_LUA_FUNCTION("GetName", [](UActorComponent* comp) -> std::string {
            return comp->GetName();
        })
        ADD_LUA_FUNCTION("IsActive", &UActorComponent::IsActive)
    END_LUA_TYPE()
}

void UScriptManager::RegisterSceneComponent(sol::state* state)
{
    // ==================== USceneComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, USceneComponent, "SceneComponent")
        // Transform - Relative
        ADD_LUA_FUNCTION("GetRelativeLocation", &USceneComponent::GetRelativeLocation)
        ADD_LUA_FUNCTION("SetRelativeLocation", &USceneComponent::SetRelativeLocation)
        ADD_LUA_FUNCTION("GetRelativeRotation", &USceneComponent::GetRelativeRotation)
        ADD_LUA_FUNCTION("SetRelativeRotationEuler", &USceneComponent::SetRelativeRotationEuler)
        ADD_LUA_FUNCTION("GetRelativeScale", &USceneComponent::GetRelativeScale)
        ADD_LUA_FUNCTION("SetRelativeScale", &USceneComponent::SetRelativeScale)

        // Transform - World
        ADD_LUA_FUNCTION("GetWorldLocation", &USceneComponent::GetWorldLocation)
        ADD_LUA_FUNCTION("SetWorldLocation", &USceneComponent::SetWorldLocation)
        ADD_LUA_FUNCTION("GetWorldRotation", &USceneComponent::GetWorldRotation)
        ADD_LUA_FUNCTION("SetWorldRotation", &USceneComponent::SetWorldRotation)
        ADD_LUA_FUNCTION("GetWorldScale", &USceneComponent::GetWorldScale)

        // Hierarchy
        ADD_LUA_FUNCTION("GetAttachParent", &USceneComponent::GetAttachParent)
        ADD_LUA_FUNCTION("GetAttachChildren", [](USceneComponent* comp, sol::this_state s) -> sol::table {
            sol::state_view lua(s);
            sol::table children = lua.create_table();
            int index = 1;
            for (USceneComponent* child : comp->GetAttachChildren()) {
                children[index++] = child;
            }
            return children;
        })
    END_LUA_TYPE()
}

void UScriptManager::RegisterStaticMeshComponent(sol::state* state)
{
    // ==================== UStaticMeshComponent 등록 ====================
    BEGIN_LUA_TYPE_WITH_BASE(state, UStaticMeshComponent, "StaticMeshComponent", UPrimitiveComponent, USceneComponent, UActorComponent)
        ADD_LUA_FUNCTION("SetStaticMesh", [](UStaticMeshComponent* comp, const std::string& path) {
            comp->SetStaticMesh(path);
        })
        ADD_LUA_FUNCTION("GetStaticMesh", &UStaticMeshComponent::GetStaticMesh)
    END_LUA_TYPE()
}

void UScriptManager::RegisterLightComponent(sol::state* state)
{
    // ==================== ULightComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, ULightComponent, "LightComponent")
        ADD_LUA_FUNCTION("SetIntensity", &ULightComponent::SetIntensity)
        ADD_LUA_FUNCTION("GetIntensity", &ULightComponent::GetIntensity)
        ADD_LUA_FUNCTION("SetLightColor", [](ULightComponent* comp, float r, float g, float b) {
            comp->SetLightColor(FLinearColor(r, g, b, 1.0f));
        })
        ADD_LUA_FUNCTION("GetLightColor", [](ULightComponent* comp, sol::this_state s) -> sol::table {
            sol::state_view lua(s);
            sol::table t = lua.create_table();
            const FLinearColor& c = comp->GetLightColor();
            // Lowercase keys for convenience
            t["r"] = c.R; t["g"] = c.G; t["b"] = c.B; t["a"] = c.A;
            // Also provide uppercase aliases
            t["R"] = c.R; t["G"] = c.G; t["B"] = c.B; t["A"] = c.A;
            return t;
        })
    END_LUA_TYPE()
}

void UScriptManager::RegisterDirectionalLightComponent(sol::state* state)
{
    // ==================== UDirectionalLightComponent 등록 ====================
    BEGIN_LUA_TYPE_WITH_BASE(state, UDirectionalLightComponent, "DirectionalLightComponent",
        ULightComponent, ULightComponentBase, USceneComponent, UActorComponent)
        // Directional-specific
        ADD_LUA_FUNCTION("GetLightDirection", &UDirectionalLightComponent::GetLightDirection)
        ADD_LUA_FUNCTION("IsOverrideCameraLightPerspective", &UDirectionalLightComponent::IsOverrideCameraLightPerspective)
    END_LUA_TYPE()
}

void UScriptManager::RegisterCameraComponent(sol::state* state)
{
    // ==================== UCameraComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UCameraComponent, "CameraComponent")
        // Camera-specific functions
        ADD_LUA_FUNCTION("SetFOV", &UCameraComponent::SetFOV)
        ADD_LUA_FUNCTION("GetFOV", &UCameraComponent::GetFOV)
        ADD_LUA_FUNCTION("SetNearClipPlane", &UCameraComponent::SetNearClipPlane)
        ADD_LUA_FUNCTION("GetNearClipPlane", &UCameraComponent::GetNearClip)
        ADD_LUA_FUNCTION("SetFarClipPlane", &UCameraComponent::SetFarClipPlane)
        ADD_LUA_FUNCTION("GetFarClipPlane", &UCameraComponent::GetFarClip)

        // SceneComponent functions (inherited but need explicit binding)
        ADD_LUA_FUNCTION("SetRelativeRotationEuler", &UCameraComponent::SetRelativeRotationEuler)
        ADD_LUA_FUNCTION("SetRelativeLocation", &UCameraComponent::SetRelativeLocation)
        ADD_LUA_FUNCTION("GetRelativeLocation", &UCameraComponent::GetRelativeLocation)
        ADD_LUA_FUNCTION("GetRelativeRotation", &UCameraComponent::GetRelativeRotation)
    END_LUA_TYPE()
}

void UScriptManager::RegisterScriptComponent(sol::state* state)
{
    // ==================== UScriptComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UScriptComponent, "ScriptComponent")
        // 스크립트 관리
        ADD_LUA_FUNCTION("SetScriptPath", &UScriptComponent::SetScriptPath)
        ADD_LUA_FUNCTION("GetScriptPath", &UScriptComponent::GetScriptPath)
        ADD_LUA_FUNCTION("ReloadScript", &UScriptComponent::ReloadScript)
        // 외부에서 스크립트의 전역 Lua 함수를 이름으로 호출
        ADD_LUA_FUNCTION("CallLuaFunction", [](UScriptComponent* comp, const std::string& funcName)
        {
            if (!comp) return;
            comp->CallLuaFunction(funcName);
        })

        // Lifecycle
        ADD_LUA_FUNCTION("BeginPlay", &UScriptComponent::BeginPlay)

        // Coroutine API
        ADD_LUA_FUNCTION("StartCoroutine", &UScriptComponent::StartCoroutine)
        ADD_LUA_FUNCTION("StopCoroutine", &UScriptComponent::StopCoroutine)
        ADD_LUA_FUNCTION("WaitForSeconds", &UScriptComponent::WaitForSeconds)
        ADD_LUA_FUNCTION("StopAllCoroutines", &UScriptComponent::StopAllCoroutines)

        // Owner Actor 접근
        ADD_LUA_FUNCTION("GetOwner", &UScriptComponent::GetOwner)
    END_LUA_TYPE()
}

void UScriptManager::RegisterInputEnums(sol::state* state)
{
    // Expose EInputAxisSource as a table for convenience
    auto axis = state->create_table("EInputAxisSource");
    axis["Key"] = static_cast<int>(EInputAxisSource::Key);
    axis["MouseX"] = static_cast<int>(EInputAxisSource::MouseX);
    axis["MouseY"] = static_cast<int>(EInputAxisSource::MouseY);
    axis["MouseWheel"] = static_cast<int>(EInputAxisSource::MouseWheel);

    // Common virtual key codes as a Keys table
    auto keys = state->create_table("Keys");
    // Letters A-Z
    for (int c = 'A'; c <= 'Z'; ++c) {
        std::string name(1, static_cast<char>(c));
        keys[name] = c;
    }
    // Digits 0-9
    for (int d = 0; d <= 9; ++d) {
        keys[std::to_string(d)] = 0x30 + d; // '0'..'9'
    }
    // Function keys F1..F12
    for (int i = 1; i <= 12; ++i) {
        keys[std::string("F") + std::to_string(i)] = 0x70 + (i - 1); // VK_F1=0x70
    }
    // Common controls
    keys["Space"]   = 0x20; // VK_SPACE
    keys["Escape"]  = 0x1B; // VK_ESCAPE
    keys["Enter"]   = 0x0D; // VK_RETURN
    keys["Tab"]     = 0x09; // VK_TAB
    keys["Backspace"] = 0x08; // VK_BACK
    keys["Shift"]   = 0x10; // VK_SHIFT
    keys["Ctrl"]    = 0x11; // VK_CONTROL
    keys["Alt"]     = 0x12; // VK_MENU
    // Arrows
    keys["Left"]  = 0x25; // VK_LEFT
    keys["Up"]    = 0x26; // VK_UP
    keys["Right"] = 0x27; // VK_RIGHT
    keys["Down"]  = 0x28; // VK_DOWN
    // Mouse buttons
    keys["MouseLeft"]   = 0x01; // VK_LBUTTON
    keys["MouseRight"]  = 0x02; // VK_RBUTTON
    keys["MouseMiddle"] = 0x04; // VK_MBUTTON
}

void UScriptManager::RegisterInputContext(sol::state* state)
{
    BEGIN_LUA_TYPE_NO_CTOR(state, UInputMappingContext, "InputContext")
        // Authoring API
        ADD_LUA_FUNCTION("MapAction", &UInputMappingContext::MapAction)
        ADD_LUA_FUNCTION("MapAxisKey", &UInputMappingContext::MapAxisKey)
        ADD_LUA_FUNCTION("MapAxisMouse", &UInputMappingContext::MapAxisMouse)

        // Lua-friendly binding helpers
        usertype["BindActionPressed"] = [](UInputMappingContext* Ctx, const FString& ActionName, sol::function Fn)
        {
            if (!Ctx || !Fn.valid()) return (FDelegateHandle)0;
            auto pf = sol::protected_function(Fn);
            return Ctx->BindActionPressed(ActionName, [pf]() mutable {
                sol::protected_function_result r = pf();
                if (!r.valid()) {
                    sol::error e = r; UE_LOG((FString("[Lua Error] ActionPressed: ") + e.what() + "\n").c_str());
                }
            });
        };
        usertype["BindActionReleased"] = [](UInputMappingContext* Ctx, const FString& ActionName, sol::function Fn)
        {
            if (!Ctx || !Fn.valid()) return (FDelegateHandle)0;
            auto pf = sol::protected_function(Fn);
            return Ctx->BindActionReleased(ActionName, [pf]() mutable {
                sol::protected_function_result r = pf();
                if (!r.valid()) {
                    sol::error e = r; UE_LOG((FString("[Lua Error] ActionReleased: ") + e.what() + "\n").c_str());
                }
            });
        };
        usertype["BindAxis"] = [](UInputMappingContext* Ctx, const FString& AxisName, sol::function Fn)
        {
            if (!Ctx || !Fn.valid()) return (FDelegateHandle)0;
            auto pf = sol::protected_function(Fn);
            return Ctx->BindAxis(AxisName, [pf](float v) mutable {
                sol::protected_function_result r = pf(v);
                if (!r.valid()) {
                    sol::error e = r; UE_LOG((FString("[Lua Error] Axis: ") + e.what() + "\n").c_str());
                }
            });
        };
    END_LUA_TYPE()

    // Factory function: Create a new InputContext as a UObject
    // DEPRECATED: Use GetPlayerController():GetInputContext() instead
    // 이 함수는 하위 호환성을 위해 남겨두었지만, 새 코드에서는 사용하지 마세요.
    state->set_function("CreateInputContext", []() -> UInputMappingContext*
    {
        return ObjectFactory::NewObject<UInputMappingContext>();
    });
}

void UScriptManager::RegisterInputSubsystem(sol::state* state)
{
    BEGIN_LUA_TYPE_NO_CTOR(state, UInputMappingSubsystem, "InputSubsystem")
        ADD_LUA_FUNCTION("AddMappingContext", &UInputMappingSubsystem::AddMappingContext)
        ADD_LUA_FUNCTION("RemoveMappingContext", &UInputMappingSubsystem::RemoveMappingContext)
        ADD_LUA_FUNCTION("ClearContexts", &UInputMappingSubsystem::ClearContexts)
        ADD_LUA_FUNCTION("WasActionPressed", &UInputMappingSubsystem::WasActionPressed)
        ADD_LUA_FUNCTION("WasActionReleased", &UInputMappingSubsystem::WasActionReleased)
        ADD_LUA_FUNCTION("IsActionDown", &UInputMappingSubsystem::IsActionDown)
        ADD_LUA_FUNCTION("GetAxisValue", &UInputMappingSubsystem::GetAxisValue)
    END_LUA_TYPE()

    // Global accessor GetInput()
    state->set_function("GetInput", []() -> UInputMappingSubsystem*
    {
        return &UInputMappingSubsystem::Get();
    });
}

void UScriptManager::RegisterGameMode(sol::state* state)
{
    UE_LOG("[UScriptManager] Registering AGameModeBase to Lua...\n");

    // ==================== AGameModeBase 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, AGameModeBase, "GameMode")
        // 게임 상태 API
        ADD_LUA_FUNCTION("GetScore", &AGameModeBase::GetScore)
        ADD_LUA_FUNCTION("SetScore", &AGameModeBase::SetScore)
        ADD_LUA_FUNCTION("AddScore", &AGameModeBase::AddScore)
        ADD_LUA_FUNCTION("GetGameTime", &AGameModeBase::GetGameTime)
        ADD_LUA_FUNCTION("GetChaserDistance", &AGameModeBase::GetChaserDistance)
        ADD_LUA_FUNCTION("SetChaserDistance", &AGameModeBase::SetChaserDistance)
        ADD_LUA_FUNCTION("IsGameOver", &AGameModeBase::IsGameOver)
        ADD_LUA_FUNCTION("EndGame", &AGameModeBase::EndGame)

        // Actor 스폰/파괴 API
        ADD_LUA_FUNCTION("SpawnActorFromLua", &AGameModeBase::SpawnActorFromLua)
        ADD_LUA_FUNCTION("DestroyActorWithEvent", &AGameModeBase::DestroyActorWithEvent)

        // ScriptComponent 접근
        ADD_LUA_FUNCTION("GetScriptComponent", &AGameModeBase::GetScriptComponent)

        // 동적 이벤트 시스템 API
        ADD_LUA_FUNCTION("RegisterEvent", &AGameModeBase::RegisterEvent)
        ADD_LUA_FUNCTION("SubscribeEvent", &AGameModeBase::SubscribeEvent)
        ADD_LUA_FUNCTION("UnsubscribeEvent", &AGameModeBase::UnsubscribeEvent)
        ADD_LUA_FUNCTION("PrintRegisteredEvents", &AGameModeBase::PrintRegisteredEvents)

        // FireEvent - sol::object를 받기 위한 커스텀 바인딩
        ADD_LUA_OVERLOAD("FireEvent",
            // 파라미터 없이 호출
            [](AGameModeBase* gm, const FString& eventName) {
                gm->FireEvent(eventName, sol::nil);
            },
            // 파라미터와 함께 호출
            [](AGameModeBase* gm, const FString& eventName, sol::object data) {
                gm->FireEvent(eventName, data);
            }
        )

        // 정적 Delegate 바인딩 (기존)
        ADD_LUA_FUNCTION("BindOnGameStart", [](AGameModeBase* gm, sol::function fn) {
            if (!gm || !fn.valid()) return (FDelegateHandle)0;
            auto FnPtr = std::make_shared<sol::protected_function>(fn);
            return gm->BindOnGameStart([FnPtr]() mutable {
                sol::protected_function_result r = (*FnPtr)();
                if (!r.valid()) {
                    sol::error e = r;
                    UE_LOG((FString("[Lua Error] OnGameStart: ") + e.what() + "\n").c_str());
                }
            });
        })

        ADD_LUA_FUNCTION("BindOnGameEnd", [](AGameModeBase* gm, sol::function fn) {
            if (!gm || !fn.valid()) return (FDelegateHandle)0;
            auto FnPtr = std::make_shared<sol::protected_function>(fn);
            return gm->BindOnGameEnd([FnPtr](bool bVictory) mutable {
                sol::protected_function_result r = (*FnPtr)(bVictory);
                if (!r.valid()) {
                    sol::error e = r;
                    UE_LOG((FString("[Lua Error] OnGameEnd: ") + e.what() + "\n").c_str());
                }
            });
        })

        ADD_LUA_FUNCTION("BindOnActorSpawned", [](AGameModeBase* gm, sol::function fn) {
            if (!gm || !fn.valid()) return (FDelegateHandle)0;
            auto FnPtr = std::make_shared<sol::protected_function>(fn);
            return gm->BindOnActorSpawned([FnPtr](AActor* actor) mutable {
                if (!actor || actor->IsPendingDestroy()) return;
                sol::protected_function_result r = (*FnPtr)(actor);
                if (!r.valid()) {
                    sol::error e = r;
                    UE_LOG((FString("[Lua Error] OnActorSpawned: ") + e.what() + "\n").c_str());
                }
            });
        })

        ADD_LUA_FUNCTION("BindOnActorDestroyed", [](AGameModeBase* gm, sol::function fn) {
            if (!gm || !fn.valid()) return (FDelegateHandle)0;

            // shared_ptr로 Lua 함수 수명 관리
            auto FnPtr = std::make_shared<sol::protected_function>(fn);

            return gm->BindOnActorDestroyed([FnPtr](AActor* actor) mutable {
                // Actor가 유효한지 확인
                if (!actor || actor->IsPendingDestroy()) {
                    UE_LOG("[Lua] OnActorDestroyed called with invalid actor\n");
                    return;
                }

                sol::protected_function_result r = (*FnPtr)(actor);
                if (!r.valid()) {
                    sol::error e = r;
                    UE_LOG((FString("[Lua Error] OnActorDestroyed: ") + e.what() + "\n").c_str());
                }
            });
        })

        ADD_LUA_FUNCTION("BindOnScoreChanged", [](AGameModeBase* gm, sol::function fn) {
            if (!gm || !fn.valid()) return (FDelegateHandle)0;
            auto FnPtr = std::make_shared<sol::protected_function>(fn);
            return gm->BindOnScoreChanged([FnPtr](int32 newScore) mutable {
                sol::protected_function_result r = (*FnPtr)(newScore);
                if (!r.valid()) {
                    sol::error e = r;
                    UE_LOG((FString("[Lua Error] OnScoreChanged: ") + e.what() + "\n").c_str());
                }
            });
        })
    END_LUA_TYPE()
}

void UScriptManager::RegisterPlayerController(sol::state* state)
{
    UE_LOG("[UScriptManager] Registering APlayerController to Lua...\n");

    // ==================== APlayerController 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, APlayerController, "PlayerController")
        // Possession API
        ADD_LUA_FUNCTION("Possess", &APlayerController::Possess)
        ADD_LUA_FUNCTION("UnPossess", &APlayerController::UnPossess)
        ADD_LUA_FUNCTION("GetPawn", &APlayerController::GetPawn)

        // ViewTarget API
        ADD_LUA_FUNCTION("SetViewTarget", &APlayerController::SetViewTarget)
        ADD_LUA_FUNCTION("GetViewTarget", &APlayerController::GetViewTarget)

        // Camera API
        ADD_LUA_FUNCTION("GetActiveCameraComponent", &APlayerController::GetActiveCameraComponent)
        ADD_LUA_FUNCTION("GetCameraLocation", &APlayerController::GetCameraLocation)
        ADD_LUA_FUNCTION("GetCameraRotation", &APlayerController::GetCameraRotation)

        // Input API
        ADD_LUA_FUNCTION("GetInputContext", &APlayerController::GetInputContext)
    END_LUA_TYPE()

    // Global accessor GetPlayerController()
    state->set_function("GetPlayerController", []() -> APlayerController*
    {
        UWorld* World = GWorld;
        if (!World) return nullptr;
        return World->GetPlayerController();
    });
}

void UScriptManager::RegisterPrimitiveComponent(sol::state* state)
{
    // ==================== UPrimitiveComponent 등록a ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UPrimitiveComponent, "PrimitiveComponent")
        // Collision settings
        ADD_LUA_FUNCTION("SetCollisionEnabled", &UPrimitiveComponent::SetCollisionEnabled)
        ADD_LUA_FUNCTION("IsCollisionEnabled", &UPrimitiveComponent::IsCollisionEnabled)
        ADD_LUA_FUNCTION("SetGenerateOverlapEvents", &UPrimitiveComponent::SetGenerateOverlapEvents)
        ADD_LUA_FUNCTION("GetGenerateOverlapEvents", &UPrimitiveComponent::GetGenerateOverlapEvents)

        // Overlap queries
        ADD_LUA_FUNCTION("IsOverlappingActor", &UPrimitiveComponent::IsOverlappingActor)

        // Overlap event bindings - Lua-friendly wrappers
        usertype["BindOnBeginOverlap"] = [](UPrimitiveComponent* Comp, sol::function Fn) -> FDelegateHandle
        {
            if (!Comp || !Fn.valid()) return (FDelegateHandle)0;

            // sol::function을 shared_ptr로 래핑하여 수명 관리
            auto FnPtr = std::make_shared<sol::protected_function>(Fn);

            // shared_ptr이 수명을 관리하므로 일반 Add() 사용 (ListenerPtr 불필요)
            return Comp->AddOnBeginOverlap(
                [FnPtr](UPrimitiveComponent* Self, AActor* OtherActor, UPrimitiveComponent* OtherComp) mutable
                {
                    sol::protected_function_result r = (*FnPtr)(Self, OtherActor, OtherComp);
                    if (!r.valid())
                    {
                        sol::error e = r;
                        UE_LOG((FString("[Lua Error] OnBeginOverlap: ") + e.what() + "\n").c_str());
                    }
                }
            );
        };

        usertype["BindOnEndOverlap"] = [](UPrimitiveComponent* Comp, sol::function Fn) -> FDelegateHandle
        {
            if (!Comp || !Fn.valid()) return (FDelegateHandle)0;

            auto FnPtr = std::make_shared<sol::protected_function>(Fn);

            // shared_ptr이 수명을 관리하므로 일반 Add() 사용 (ListenerPtr 불필요)
            return Comp->AddOnEndOverlap(
                [FnPtr](UPrimitiveComponent* Self, AActor* OtherActor, UPrimitiveComponent* OtherComp) mutable
                {
                    sol::protected_function_result r = (*FnPtr)(Self, OtherActor, OtherComp);
                    if (!r.valid())
                    {
                        sol::error e = r;
                        UE_LOG((FString("[Lua Error] OnEndOverlap: ") + e.what() + "\n").c_str());
                    }
                }
            );
        };

        ADD_LUA_FUNCTION("UnbindOnBeginOverlap", &UPrimitiveComponent::RemoveOnBeginOverlap)
        ADD_LUA_FUNCTION("UnbindOnEndOverlap", &UPrimitiveComponent::RemoveOnEndOverlap)
    END_LUA_TYPE()
}

void UScriptManager::RegisterShapeComponent(sol::state* state)
{
    // ==================== UShapeComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UShapeComponent, "ShapeComponent")
        // Shape-specific properties
        ADD_LUA_PROPERTY("ShapeColor", &UShapeComponent::ShapeColor)
        ADD_LUA_PROPERTY("bDrawOnlyIfSelected", &UShapeComponent::bDrawOnlyIfSelected)

        // Overlap test
        ADD_LUA_FUNCTION("Overlaps", &UShapeComponent::Overlaps)
    END_LUA_TYPE()
}

void UScriptManager::RegisterBoxComponent(sol::state* state)
{
    BEGIN_LUA_TYPE_WITH_BASE(state, UBoxComponent, "BoxComponent",
        UShapeComponent, UPrimitiveComponent, USceneComponent, UActorComponent)
        ADD_LUA_PROPERTY("BoxExtent", &UBoxComponent::BoxExtent)
    END_LUA_TYPE()

    // 반드시 루아스크립트를 통해서 사이즈 인자를 받아야 합니다!
    state->set_function("AddBoxComponent",
        [](AActor* Owner, const FVector& extent) -> UBoxComponent*
        {
            if (!Owner) return nullptr;

            UBoxComponent* Comp = ObjectFactory::NewObject<UBoxComponent>();
            if (!Comp) return nullptr;

            Comp->BoxExtent = extent;
            Owner->AddOwnedComponent(Comp);

            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                Comp->SetupAttachment(Root, EAttachmentRule::KeepRelative);
            }

            if (UWorld* World = Owner->GetWorld())
            {
                Comp->RegisterComponent(World);
                Comp->InitializeComponent();
            }

            Comp->SetCollisionEnabled(true);
            Comp->SetGenerateOverlapEvents(true);

            return Comp;
        }
    );
}

void UScriptManager::RegisterProjectileMovement(sol::state* state)
{
    BEGIN_LUA_TYPE_NO_CTOR(state, UProjectileMovementComponent, "ProjectileMovement")
        // Movement base API
        // Use lambdas to avoid any base-class binding ambiguity and ensure correct dispatch
        usertype["SetVelocity"] = [](UProjectileMovementComponent* Self, const FVector& V)
        {
            if (!Self) return;
            Self->SetVelocity(V);
        };
        usertype["GetVelocity"] = [](UProjectileMovementComponent* Self) -> FVector
        {
            return Self ? Self->GetVelocity() : FVector(0,0,0);
        };
        usertype["SetAcceleration"] = [](UProjectileMovementComponent* Self, const FVector& A)
        {
            if (!Self) return;
            Self->SetAcceleration(A);
        };
        usertype["GetAcceleration"] = [](UProjectileMovementComponent* Self) -> FVector
        {
            return Self ? Self->GetAcceleration() : FVector(0,0,0);
        };
        usertype["StopMovement"] = [](UProjectileMovementComponent* Self)
        {
            if (!Self) return;
            Self->StopMovement();
        };

        // Projectile specific
        ADD_LUA_FUNCTION("SetGravity", &UProjectileMovementComponent::SetGravity)
        ADD_LUA_FUNCTION("GetGravity", &UProjectileMovementComponent::GetGravity)
        ADD_LUA_FUNCTION("SetMaxSpeed", &UProjectileMovementComponent::SetMaxSpeed)
        ADD_LUA_FUNCTION("GetMaxSpeed", &UProjectileMovementComponent::GetMaxSpeed)
        ADD_LUA_FUNCTION("SetRotationFollowsVelocity", &UProjectileMovementComponent::SetRotationFollowsVelocity)
        ADD_LUA_FUNCTION("GetRotationFollowsVelocity", &UProjectileMovementComponent::GetRotationFollowsVelocity)
        ADD_LUA_FUNCTION("SetProjectileLifespan", &UProjectileMovementComponent::SetProjectileLifespan)
        ADD_LUA_FUNCTION("GetProjectileLifespan", &UProjectileMovementComponent::GetProjectileLifespan)
        ADD_LUA_FUNCTION("SetAutoDestroyWhenLifespanExceeded", &UProjectileMovementComponent::SetAutoDestroyWhenLifespanExceeded)
        ADD_LUA_FUNCTION("GetAutoDestroyWhenLifespanExceeded", &UProjectileMovementComponent::GetAutoDestroyWhenLifespanExceeded)
        ADD_LUA_FUNCTION("FireInDirection", &UProjectileMovementComponent::FireInDirection)

        // Convenience: set updated to owner's root
        usertype["SetUpdatedToOwnerRoot"] = [](UProjectileMovementComponent* C)
        {
            if (!C) return;
            AActor* Owner = C->GetOwner();
            if (!Owner) return;
            USceneComponent* Root = Owner->GetRootComponent();
            if (Root) C->SetUpdatedComponent(Root);
        };
    END_LUA_TYPE()

    // Factory: add to an actor and set updated to root; register immediately
    state->set_function("AddProjectileMovement", sol::overload(
        [](AActor* Owner) -> UProjectileMovementComponent*
        {
            if (!Owner) return nullptr;
            UProjectileMovementComponent* Comp = ObjectFactory::NewObject<UProjectileMovementComponent>();
            Owner->AddOwnedComponent(Comp);
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                Comp->SetUpdatedComponent(Root);
            }
            if (UWorld* World = Owner->GetWorld())
            {
                Comp->RegisterComponent(World);
                Comp->InitializeComponent();
                if (World->bPie) { Comp->BeginPlay(); }
            }
            return Comp;
        },
        [](sol::this_state ts, sol::this_environment env) -> UProjectileMovementComponent*
        {
            // environment에서 actor를 찾음 (독립 environment 지원)
            sol::environment current_env = env;
            sol::optional<AActor*> OwnerOpt = current_env["actor"];

            // environment에서 못 찾으면 전역 state에서 찾기
            if (!OwnerOpt)
            {
                sol::state_view sv(ts);
                OwnerOpt = sv["actor"];
            }
            if (!OwnerOpt) return nullptr;

            AActor* Owner = OwnerOpt.value();
            if (!Owner) return nullptr;
            UProjectileMovementComponent* Comp = ObjectFactory::NewObject<UProjectileMovementComponent>();
            Owner->AddOwnedComponent(Comp);
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                Comp->SetUpdatedComponent(Root);
            }
            if (UWorld* World = Owner->GetWorld())
            {
                Comp->RegisterComponent(World);
                Comp->InitializeComponent();
                if (World->bPie) { Comp->BeginPlay(); }
            }
            return Comp;
        }
    ));
}

void UScriptManager::RegisterRotatingMovement(sol::state* state)
{
    BEGIN_LUA_TYPE_NO_CTOR(state, URotatingMovementComponent, "RotatingMovement")
        // Movement base API
        // Use lambdas to avoid any base-class binding ambiguity and ensure correct dispatch
        usertype["SetVelocity"] = [](URotatingMovementComponent* Self, const FVector& V)
        {
            if (!Self) return;
            Self->SetVelocity(V);
        };
        usertype["GetVelocity"] = [](URotatingMovementComponent* Self) -> FVector
        {
            return Self ? Self->GetVelocity() : FVector(0,0,0);
        };

        // Rotating specific
        ADD_LUA_FUNCTION("SetRotationRate", &URotatingMovementComponent::SetRotationRate)
        ADD_LUA_FUNCTION("GetRotationRate", &URotatingMovementComponent::GetRotationRate)

        // Convenience: set updated to owner's root
        usertype["SetUpdatedToOwnerRoot"] = [](URotatingMovementComponent* C)
        {
            if (!C) return;
            AActor* Owner = C->GetOwner();
            if (!Owner) return;
            USceneComponent* Root = Owner->GetRootComponent();
            if (Root) C->SetUpdatedComponent(Root);
        };
    END_LUA_TYPE()

    // Factory: add to an actor and set updated to root; register immediately
    state->set_function("AddRotatingMovement", sol::overload(
        [](AActor* Owner) -> URotatingMovementComponent*
        {
            if (!Owner) return nullptr;
            URotatingMovementComponent* Comp = ObjectFactory::NewObject<URotatingMovementComponent>();
            Owner->AddOwnedComponent(Comp);
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                Comp->SetUpdatedComponent(Root);
            }
            if (UWorld* World = Owner->GetWorld())
            {
                Comp->RegisterComponent(World);
                Comp->InitializeComponent();
                if (World->bPie) { Comp->BeginPlay(); }
            }
            return Comp;
        },
        [](sol::this_state ts, sol::this_environment env) -> URotatingMovementComponent*
        {
            // environment에서 actor를 찾음 (독립 environment 지원)
            sol::environment current_env = env;
            sol::optional<AActor*> OwnerOpt = current_env["actor"];

            // environment에서 못 찾으면 전역 state에서 찾기
            if (!OwnerOpt)
            {
                sol::state_view sv(ts);
                OwnerOpt = sv["actor"];
            }
            if (!OwnerOpt) return nullptr;

            AActor* Owner = OwnerOpt.value();
            if (!Owner) return nullptr;
            URotatingMovementComponent* Comp = ObjectFactory::NewObject<URotatingMovementComponent>();
            Owner->AddOwnedComponent(Comp);
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                Comp->SetUpdatedComponent(Root);
            }
            if (UWorld* World = Owner->GetWorld())
            {
                Comp->RegisterComponent(World);
                Comp->InitializeComponent();
                if (World->bPie) { Comp->BeginPlay(); }
            }
            return Comp;
        }
    ));
}
