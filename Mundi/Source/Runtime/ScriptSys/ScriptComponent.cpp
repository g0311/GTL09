#include "pch.h"
#include "ScriptComponent.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "ImGui/imgui.h"
#include <filesystem>

IMPLEMENT_CLASS(UScriptComponent)

BEGIN_PROPERTIES(UScriptComponent)
    MARK_AS_COMPONENT("스크립트 컴포넌트", "루아 스크립트")
    ADD_PROPERTY_SCRIPTPATH(FString, ScriptPath, "Script", true, "루아 스크립트 파일 경로")
END_PROPERTIES()
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
    UActorComponent::BeginPlay();

    // 스크립트 로드
    if (!bScriptLoaded && !ScriptPath.empty())
    {
        ReloadScript();
    }
    
    // Lua BeginPlay() 호출
    if (bScriptLoaded)
    {
        sol::state& lua = UScriptManager::GetInstance().GetLuaState();
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
    UActorComponent::TickComponent(DeltaTime);

    if (!bScriptLoaded)
        return;
    
    // Lua Tick(dt) 호출
    sol::state& lua = UScriptManager::GetInstance().GetLuaState();
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
        sol::state& lua = UScriptManager::GetInstance().GetLuaState();
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
    
    UActorComponent::EndPlay(Reason);
}

// ==================== 스크립트 관리 ====================

bool UScriptComponent::SetScriptPath(const FString& InScriptPath)
{
    ScriptPath = InScriptPath;
    return ReloadScript();
}

void UScriptComponent::OpenScriptInEditor()
{
    if (ScriptPath.empty())
    {
        OutputDebugStringA("No script path set\n");
        return;
    }
    
    // 상대 경로를 절대 경로로 변환
    namespace fs = std::filesystem;
    fs::path absolutePath;
    
    if (fs::path(ScriptPath).is_absolute())
    {
        absolutePath = ScriptPath;
    }
    else
    {
        absolutePath = fs::current_path() / ScriptPath;
    }
    
    // 파일 존재 확인
    if (!fs::exists(absolutePath))
    {
        FString errorMsg = "Script file not found: " + absolutePath.string();
        MessageBoxA(NULL, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Windows 기본 에디터로 열기
    HINSTANCE hInst = ShellExecuteA(
        NULL,
        "open",
        absolutePath.string().c_str(),
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
    
    sol::state& lua = UScriptManager::GetInstance().GetLuaState();
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
    
    sol::state& lua = UScriptManager::GetInstance().GetLuaState();
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

void UScriptComponent::OnSerialized()
{
    Super::OnSerialized();

    // 직렬화 이후 스크립트 경로가 세팅되어 있으면 필요 시 재로딩
    if (!ScriptPath.empty())
    {
        bScriptLoaded = false;
        ReloadScript();
    }
}

void UScriptComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // 복제본은 런타임 로드 상태를 초기화하고 필요 시 BeginPlay/OnSerialized에서 로드
    bScriptLoaded = false;
}
