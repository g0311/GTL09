#include "pch.h"
#include "ScriptComponent.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "CoroutineHelper.h"
#include <filesystem>
#include <chrono>

IMPLEMENT_CLASS(UScriptComponent)

BEGIN_PROPERTIES(UScriptComponent)
	MARK_AS_COMPONENT("스크립트 컴포넌트", "루아 스크립트")
	ADD_PROPERTY_SCRIPTPATH(FString, ScriptPath, "Script", true, "루아 스크립트 파일 경로")
END_PROPERTIES()

// ==================== Construction / Destruction ====================
UScriptComponent::UScriptComponent()
{
    // 틱 활성화
    SetCanEverTick(true);
    SetTickEnabled(true);

    lua = new sol::state;

	EnsureCoroutineHelper();
}

UScriptComponent::~UScriptComponent()
{
	StopAllCoroutines();

    if (CoroutineHelper)
    {
        delete CoroutineHelper;
        CoroutineHelper = nullptr;
    }

    delete lua;
}

// ==================== Lifecycle ====================
void UScriptComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	EnsureCoroutineHelper();

    // 스크립트 로드
    if (!bScriptLoaded && !ScriptPath.empty())
    {
        ReloadScript();
    }
    
    // Lua BeginPlay() 호출
    if (bScriptLoaded)
    {
        sol::protected_function func = (*lua)["BeginPlay"];
        if (func.valid())
        {
            auto result = func();
            if (!result.valid())
            {
                sol::error err = result;
                UE_LOG(("BeginPlay error: " + std::string(err.what()) + "\n").c_str());
            }
        }
    }
}

void UScriptComponent::TickComponent(float DeltaTime)
{
	UActorComponent::TickComponent(DeltaTime);

    HotReloadCheckTimer += DeltaTime;
    if (HotReloadCheckTimer > 1.0f)
    {
        HotReloadCheckTimer = 0.0f;
        if (!ScriptPath.empty() && std::filesystem::exists(ScriptPath))
        {
            try
            {
                auto ftime = std::filesystem::last_write_time(ScriptPath);
                long long currentTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch()).count();

                if (currentTime_ms > LastScriptWriteTime_ms)
                {
                    ReloadScript();
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                UE_LOG(e.what());
            }
        }
    }
    if (!bScriptLoaded)
        return;
    
    // Lua Tick(dt) 호출
    sol::protected_function func = (*lua)["Tick"];
    if (func.valid())
    {
        auto result = func(DeltaTime);
        if (!result.valid())
        {
            sol::error err = result;
            UE_LOG(("Tick error: " + std::string(err.what()) + "\n").c_str());
        }
    }

    if (CoroutineHelper)
    {
        CoroutineHelper->RunScheduler(DeltaTime);
    }
}

void UScriptComponent::EndPlay(EEndPlayReason Reason)
{
    // Lua EndPlay() 호출
    if (bScriptLoaded)
    {
        sol::protected_function func = (*lua)["EndPlay"];
        if (func.valid())
        {
            auto result = func();
            if (!result.valid())
            {
                sol::error err = result;
                UE_LOG(("EndPlay error: " + std::string(err.what()) + "\n").c_str());
            }
        }
    }
    
    StopAllCoroutines();

    UActorComponent::EndPlay(Reason);
}

// ==================== Script Management ====================
bool UScriptComponent::SetScriptPath(const FString& InScriptPath)
{
	ScriptPath = InScriptPath;
	return ReloadScript();
}

void UScriptComponent::OpenScriptInEditor()
{
    if (ScriptPath.empty())
    {
        UE_LOG("No script path set\n");
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
        UE_LOG(("Script file not found: " + ScriptPath + "\n").c_str());
        bScriptLoaded = false;
        return false;
    }
    
    // Owner Actor를 Lua에 바인딩
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        UE_LOG("No owner actor\n");
        return false;
    }

    StopAllCoroutines();
    
    UScriptManager::GetInstance().RegisterTypesToState(lua);
    
    // 이 컴포넌트 전용 변수 바인딩
    (*lua)["actor"] = OwnerActor;
    (*lua)["self"] = this;
    
    // 스크립트 로드
    try
    {
        lua->script_file(ScriptPath);
        bScriptLoaded = true;

        // Store timestamp for hot-reload
        auto ftime = std::filesystem::last_write_time(ScriptPath);
        LastScriptWriteTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch()).count();

        UE_LOG(("Loaded script: " + ScriptPath + "\n").c_str());
        return true;
    }
    catch (const sol::error& e)
    {
        UE_LOG(("Lua error: " + std::string(e.what()) + "\n").c_str());
        bScriptLoaded = false;
        return false;
    }
}

// ==================== Coroutine ====================
void UScriptComponent::StartCoroutine(sol::function EntryPoint)
{
	EnsureCoroutineHelper();
	if (CoroutineHelper)
	{
		CoroutineHelper->StartCoroutine(std::move(EntryPoint));
	}
}

FYieldInstruction* UScriptComponent::WaitForSeconds(float Seconds)
{
	EnsureCoroutineHelper();
	return CoroutineHelper ? CoroutineHelper->CreateWaitForSeconds(Seconds) : nullptr;
}

void UScriptComponent::StopAllCoroutines()
{
	if (CoroutineHelper)
	{
		CoroutineHelper->Stop();
	}
}

void UScriptComponent::EnsureCoroutineHelper()
{
	if (!CoroutineHelper)
	{
		CoroutineHelper = new FCoroutineHelper(this);
	}
}

// ==================== Lua Events ====================
void UScriptComponent::NotifyOverlap(AActor* OtherActor)
{
    if (!bScriptLoaded || !OtherActor)
        return;
    
    sol::protected_function func = (*lua)["OnOverlap"];
    if (func.valid())
    {
        auto result = func(OtherActor);
        if (!result.valid())
        {
            sol::error err = result;
            UE_LOG(("OnOverlap error: " + std::string(err.what()) + "\n").c_str());
        }
    }
}

// ==================== Serialization ====================
void UScriptComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UActorComponent::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadString(InOutHandle, "ScriptPath", ScriptPath);
	}
	else
	{
		InOutHandle["ScriptPath"] = ScriptPath.c_str();
	}
}

void UScriptComponent::OnSerialized()
{
	Super::OnSerialized();

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
	CoroutineHelper = nullptr;
    lua = new sol::state;
}

void UScriptComponent::PostDuplicate()
{
	Super::PostDuplicate();
	EnsureCoroutineHelper();
}
