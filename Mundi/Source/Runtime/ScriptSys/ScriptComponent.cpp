#include "pch.h"
#include "ScriptComponent.h"
#include "Actor.h"
#include "SceneComponent.h"
#include <filesystem>

IMPLEMENT_CLASS(UScriptComponent)

// ==================== 생성자/소멸자 ====================

UScriptComponent::UScriptComponent()
{
    // 틱 활성화
    SetCanEverTick(true);
    SetTickEnabled(true);
}

UScriptComponent::~UScriptComponent()
{
}

// ==================== Lifecycle ====================

void UScriptComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Lua state 초기화
    if (!bScriptLoaded && !ScriptPath.empty())
    {
        InitializeLuaState();
        ReloadScript();
    }
    
    // Lua BeginPlay() 호출
    if (bScriptLoaded)
    {
        sol::protected_function func = lua["BeginPlay"];
        if (func.valid())
        {
            auto result = func();
            if (!result.valid())
            {
                sol::error err = result;
                OutputDebugStringA(("BeginPlay error: " + std::string(err.what()) + "\n").c_str());
            }
        }
    }
}

void UScriptComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    
    if (!bScriptLoaded)
        return;
    
    // Lua Tick(dt) 호출
    sol::protected_function func = lua["Tick"];
    if (func.valid())
    {
        auto result = func(DeltaTime);
        if (!result.valid())
        {
            sol::error err = result;
            OutputDebugStringA(("Tick error: " + std::string(err.what()) + "\n").c_str());
        }
    }
}

void UScriptComponent::EndPlay(EEndPlayReason Reason)
{
    // Lua EndPlay() 호출
    if (bScriptLoaded)
    {
        sol::protected_function func = lua["EndPlay"];
        if (func.valid())
        {
            auto result = func();
            if (!result.valid())
            {
                sol::error err = result;
                OutputDebugStringA(("EndPlay error: " + std::string(err.what()) + "\n").c_str());
            }
        }
    }
    
    Super::EndPlay(Reason);
}

// ==================== 스크립트 관리 ====================

bool UScriptComponent::SetScriptPath(const FString& InScriptPath)
{
    ScriptPath = InScriptPath;
    
    // Lua state 초기화 (처음 호출 시)
    if (lua.lua_state() == nullptr)
    {
        InitializeLuaState();
    }
    
    return ReloadScript();
}

void UScriptComponent::OpenScriptInEditor()
{
    if (ScriptPath.empty())
    {
        OutputDebugStringA("No script path set\n");
        return;
    }
    
    // Windows 기본 에디터로 열기
    HINSTANCE hInst = ShellExecuteA(
        NULL,
        "open",
        ScriptPath.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
    
    if ((INT_PTR)hInst <= 32)
    {
        MessageBoxA(NULL, "Failed to open script file", "Error", MB_OK | MB_ICONERROR);
    }
}

bool UScriptComponent::ReloadScript()
{
    if (ScriptPath.empty())
        return false;
    
    namespace fs = std::filesystem;
    if (!fs::exists(ScriptPath))
    {
        OutputDebugStringA(("Script file not found: " + ScriptPath + "\n").c_str());
        bScriptLoaded = false;
        return false;
    }
    
    // Owner Actor를 Lua에 바인딩
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        OutputDebugStringA("No owner actor\n");
        return false;
    }
    
    lua["actor"] = OwnerActor;
    lua["self"] = this;  // ScriptComponent 자신도 노출
    
    // 스크립트 로드
    try
    {
        lua.script_file(ScriptPath);
        bScriptLoaded = true;
        OutputDebugStringA(("Loaded script: " + ScriptPath + "\n").c_str());
        return true;
    }
    catch (const sol::error& e)
    {
        OutputDebugStringA(("Lua error: " + std::string(e.what()) + "\n").c_str());
        bScriptLoaded = false;
        return false;
    }
}

// ==================== Lua 이벤트 ====================

void UScriptComponent::NotifyOverlap(AActor* OtherActor)
{
    if (!bScriptLoaded || !OtherActor)
        return;
    
    sol::protected_function func = lua["OnOverlap"];
    if (func.valid())
    {
        auto result = func(OtherActor);
        if (!result.valid())
        {
            sol::error err = result;
            OutputDebugStringA(("OnOverlap error: " + std::string(err.what()) + "\n").c_str());
        }
    }
}

// ==================== 초기화 ====================

void UScriptComponent::InitializeLuaState()
{
    // Lua 기본 라이브러리 로드
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    
    // print를 OutputDebugString으로 리다이렉트
    lua.set_function("print", [](const std::string& msg) {
        OutputDebugStringA((msg + "\n").c_str());
    });
    
    // 타입 등록
    RegisterTypes();
}

void UScriptComponent::RegisterTypes()
{
    // ==================== FVector 등록 ====================
    lua.new_usertype<FVector>("Vector",
        sol::constructors<FVector(), FVector(float, float, float)>(),
        "X", &FVector::X,
        "Y", &FVector::Y,
        "Z", &FVector::Z,
        sol::meta_function::addition, [](const FVector& a, const FVector& b) {
            return a + b;
        },
        sol::meta_function::multiplication, [](const FVector& v, float f) {
            return v * f;
        }
    );
    
    // ==================== FQuat 등록 ====================
    lua.new_usertype<FQuat>("Quat",
        sol::constructors<FQuat()>(),
        "X", &FQuat::X,
        "Y", &FQuat::Y,
        "Z", &FQuat::Z,
        "W", &FQuat::W
    );
    
    // ==================== FTransform 등록 ====================
    lua.new_usertype<FTransform>("Transform",
        "Location", &FTransform::Translation,
        "Rotation", &FTransform::Rotation,
        "Scale", &FTransform::Scale3D
    );
    
    // ==================== AActor 등록 ====================
    lua.new_usertype<AActor>("Actor",
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
        
        // Direction API
        "GetActorForward", &AActor::GetActorForward,
        "GetActorRight", &AActor::GetActorRight,
        "GetActorUp", &AActor::GetActorUp,
        
        // Name/Visibility
        "GetName", &AActor::GetName,
        "SetActorHiddenInGame", &AActor::SetActorHiddenInGame,
        "GetActorHiddenInGame", &AActor::GetActorHiddenInGame
    );
    
    // ==================== UScriptComponent 등록 ====================
    lua.new_usertype<UScriptComponent>("ScriptComponent",
        "GetScriptPath", &UScriptComponent::GetScriptPath,
        "ReloadScript", &UScriptComponent::ReloadScript,
        "OpenScriptInEditor", &UScriptComponent::OpenScriptInEditor
    );
}

// ==================== Serialize ====================

void UScriptComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UActorComponent::Serialize(bInIsLoading, InOutHandle);
    
    if (bInIsLoading)
    {
        // 로드 시: ScriptPath를 읽어옴
        FJsonSerializer::ReadString(InOutHandle, "ScriptPath", ScriptPath);
    }
    else
    {
        // 저장 시: ScriptPath를 기록
        InOutHandle["ScriptPath"] = ScriptPath.c_str();
    }
}
