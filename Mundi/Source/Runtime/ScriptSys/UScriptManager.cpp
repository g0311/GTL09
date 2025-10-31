#include "pch.h"
#include "sol/sol.hpp"
#include "UScriptManager.h"
#include "Actor.h"
#include <filesystem>
#include "ScriptUtils.h"

// ==================== 초기화 ====================
IMPLEMENT_CLASS(UScriptManager)

void UScriptManager::RegisterTypesToState(sol::state* state)
{
    if (!state) return;

    RegisterCoreTypes(state);
    
    RegisterActor(state);
}
void UScriptManager::RegisterLOG(sol::state* state)
{
    // ==================== Log 등록 ====================
    state->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
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

void UScriptManager::RegisterCoreTypes(sol::state* state)
{
    RegisterLOG(state);
    RegisterVector(state);
    RegisterQuat(state);
    RegisterTransform(state);
}

void UScriptManager::RegisterReflectedClasses(sol::state* state)
{
}

void UScriptManager::RegisterVector(sol::state* state)
{
    // ==================== FVector 등록 ====================
    BEGIN_LUA_TYPE(state, FVector, "Vector", FVector(), FVector(float, float, float))
        ADD_LUA_PROPERTY("X", &FVector::X)
        ADD_LUA_PROPERTY("Y", &FVector::Y)
        ADD_LUA_PROPERTY("Z", &FVector::Z)
        ADD_LUA_META_FUNCTION(addition, [](const FVector& a, const FVector& b) {
            return a + b;
        })
        ADD_LUA_META_FUNCTION(subtraction, [](const FVector& a, const FVector& b) {
            return a - b;
        })
        ADD_LUA_OVERLOAD(sol::meta_function::multiplication,
            sol::overload(
                [](const FVector& v, float f) { return v * f; },
                [](float f, const FVector& v) { return v * f; }
            )
        )
        ADD_LUA_META_FUNCTION(division, [](const FVector& v, float f) {
            return v / f;
        })
    END_LUA_TYPE() // ★ 4. (가독성용) 종료
}

void UScriptManager::RegisterQuat(sol::state* state)
{
    // ==================== FQuat 등록 ====================
    BEGIN_LUA_TYPE(state, FQuat, "Quat", FQuat(), FQuat())
        ADD_LUA_PROPERTY("X", &FQuat::X)
        ADD_LUA_PROPERTY("Y", &FQuat::Y)
        ADD_LUA_PROPERTY("Z", &FQuat::Z)
        ADD_LUA_PROPERTY("W", &FQuat::W)
    END_LUA_TYPE()
}

void UScriptManager::RegisterTransform(sol::state* state)
{
    // ==================== FTransform 등록 ====================
    BEGIN_LUA_TYPE(state, FTransform, "Transform", FTransform(), FTransform())
        ADD_LUA_PROPERTY("Location", &FTransform::Translation)
        ADD_LUA_PROPERTY("Rotation", &FTransform::Rotation)
        ADD_LUA_PROPERTY("Scale", &FTransform::Scale3D)
    END_LUA_TYPE()
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
        
        // Direction API
        ADD_LUA_FUNCTION("GetActorForward", &AActor::GetActorForward)
        ADD_LUA_FUNCTION("GetActorRight", &AActor::GetActorRight)
        ADD_LUA_FUNCTION("GetActorUp", &AActor::GetActorUp)
        
        // Name/Visibility
        ADD_LUA_FUNCTION("GetName", &AActor::GetName)
        ADD_LUA_FUNCTION("SetActorHiddenInGame", &AActor::SetActorHiddenInGame)
        ADD_LUA_FUNCTION("GetActorHiddenInGame", &AActor::GetActorHiddenInGame)
        
        // Lifecycle
        ADD_LUA_FUNCTION("Destroy", &AActor::Destroy)
    END_LUA_TYPE()
}

// ==================== 스크립트 파일 관리 ====================

bool UScriptManager::CreateScript(const FString& SceneName, const FString& ActorName)
{
    namespace fs = std::filesystem;

    // 현재 작업 디렉토리 기준 절대 경로 생성
    fs::path templatePath = fs::current_path() / "Source" / "Runtime" / "ScriptSys" / "template.lua";
    
    // 새 스크립트 파일명 생성: SceneName_ActorName.lua
    FString scriptName = SceneName + "_" + ActorName + ".lua";
    fs::path newScriptPath = fs::current_path() / "LuaScripts" / scriptName;

    // 1. 템플릿 파일 존재 확인
    if (!fs::exists(templatePath))
    {
        OutputDebugStringA("Error: template.lua not found\n");
        return false;
    }

    // 2. 이미 파일이 존재하는 경우 성공 반환 (중복 생성 방지)
    if (fs::exists(newScriptPath))
    {
        OutputDebugStringA(("Script already exists: " + scriptName + "\n").c_str());
        return true;
    }

    // 3. 템플릿 파일을 새 이름으로 복사
    try
    {
        fs::copy_file(templatePath, newScriptPath);
        OutputDebugStringA(("Created script: " + scriptName + "\n").c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(("Error creating script: " + std::string(e.what()) + "\n").c_str());
        return false;
    }
}

void UScriptManager::EditScript(const FString& ScriptPath)
{
    // Windows 확장자 연결을 사용하여 .lua 파일을 기본 프로그램으로 열기
    HINSTANCE hInst = ShellExecuteA(
        NULL,                   // 부모 윈도우 핸들
        "open",                 // 동작 (open = 열기)
        ScriptPath.c_str(),     // 열 파일 경로
        NULL,                   // 명령줄 인자
        NULL,                   // 작업 디렉토리
        SW_SHOWNORMAL           // 창 표시 상태
    );

    // ShellExecute는 성공 시 32보다 큰 값 반환
    if ((INT_PTR)hInst <= 32)
    {
        MessageBoxA(NULL, "Failed to open script file", "Error", MB_OK | MB_ICONERROR);
    }
}
