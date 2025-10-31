#include "pch.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "ScriptComponent.h"
#include <filesystem>

// ==================== 초기화 ====================
IMPLEMENT_CLASS(UScriptManager)

void UScriptManager::RegisterTypesToState(sol::state* state)
{
    if (!state) return;

    
}

void UScriptManager::RegisterGlobalTypes(sol::state* state)
{
    if (!state)
        return;
    // ==================== FVector 등록 ====================
    state->new_usertype<FVector>("Vector",
        sol::constructors<FVector(), FVector(float, float, float)>(),
        "X", &FVector::X,
        "Y", &FVector::Y,
        "Z", &FVector::Z,
        sol::meta_function::addition, [](const FVector& a, const FVector& b) {
            return a + b;
        },
        sol::meta_function::subtraction, [](const FVector& a, const FVector& b) {
            return a - b;
        },
        sol::meta_function::multiplication, sol::overload(
            [](const FVector& v, float f) { return v * f; },
            [](float f, const FVector& v) { return v * f; }
        ),
        sol::meta_function::division, [](const FVector& v, float f) {
            return v / f;
        }
    );
    
    // ==================== FQuat 등록 ====================
    state->new_usertype<FQuat>("Quat",
        sol::constructors<FQuat()>(),
        "X", &FQuat::X,
        "Y", &FQuat::Y,
        "Z", &FQuat::Z,
        "W", &FQuat::W
    );
    
    // ==================== FTransform 등록 ====================
    state->new_usertype<FTransform>("Transform",
        "Location", &FTransform::Translation,
        "Rotation", &FTransform::Rotation,
        "Scale", &FTransform::Scale3D
    );
    
    // ==================== AActor 등록 ====================
    state->new_usertype<AActor>("Actor",
        // Transform API
        "GetActorLocation", &AActor::GetActorLocation,
        "SetActorLocation", &AActor::SetActorLocation,
        "GetActorRotation", &AActor::GetActorRotation,
        "SetActorRotation", sol::overload(
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::SetActorRotation),
            static_cast<void(AActor::*)(const FVector&)>(&AActor::SetActorRotation)
        ),
        "GetActorScale", &AActor::GetActorScale,
        "SetActorScale", &AActor::SetActorScale,
        
        // Movement API
        "AddActorWorldLocation", &AActor::AddActorWorldLocation,
        "AddActorLocalLocation", &AActor::AddActorLocalLocation,
        "AddActorWorldRotation", sol::overload(
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorWorldRotation),
            static_cast<void(AActor::*)(const FVector&)>(&AActor::AddActorWorldRotation)
        ),
        "AddActorLocalRotation", sol::overload(
            static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorLocalRotation),
            static_cast<void(AActor::*)(const FVector&)>(&AActor::AddActorLocalRotation)
        ),
        
        // Direction API
        "GetActorForward", &AActor::GetActorForward,
        "GetActorRight", &AActor::GetActorRight,
        "GetActorUp", &AActor::GetActorUp,
        
        // Name/Visibility
        "GetName", &AActor::GetName,
        "SetActorHiddenInGame", &AActor::SetActorHiddenInGame,
        "GetActorHiddenInGame", &AActor::GetActorHiddenInGame,
        
        // Lifecycle
        "Destroy", &AActor::Destroy
    );
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
