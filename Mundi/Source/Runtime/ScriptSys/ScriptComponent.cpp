#include "pch.h"
#include "ScriptComponent.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "Source/Runtime/Core/Game/CoroutineHelper.h"

IMPLEMENT_CLASS(UScriptComponent)

BEGIN_PROPERTIES(UScriptComponent)
	MARK_AS_COMPONENT("스크립트 컴포넌트", "루아 스크립트")
	ADD_PROPERTY_SCRIPTPATH(FString, ScriptPath, "Script", true, "루아 스크립트 파일 경로")
END_PROPERTIES()

// ==================== Construction / Destruction ====================
UScriptComponent::UScriptComponent()
{
    SetCanEverTick(true);
    SetTickEnabled(true);

	// Lua state 생성
	Lua = new sol::state();

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

    delete Lua;
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
        CallLuaFunction("BeginPlay");
    }
}

void UScriptComponent::TickComponent(float DeltaTime)
{
	UActorComponent::TickComponent(DeltaTime);

    // 1. 핫 리로드 체크
    CheckHotReload(DeltaTime);

    // Case A. 스크립트가 존재하지 않으면 Tick 생략
    if (!bScriptLoaded) { return; }
    
    // 2. Lua Tick 호출
    CallLuaFunction("Tick", DeltaTime);

    // 3. 코루틴 실행
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
        CallLuaFunction("EndPlay");
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

    // 1. UScriptManager를 통해서 파일을 Read 합니다.
    UScriptManager::GetInstance().EditScript(ScriptPath);
}

bool UScriptComponent::ReloadScript()
{
    if (ScriptPath.empty())
        return false;

    namespace fs = std::filesystem;

    fs::path AbsolutePath = UScriptManager::ResolveScriptPath(ScriptPath);

    if (!fs::exists(AbsolutePath))
    {
        UE_LOG("Script file not found\n");
        UE_LOG(("  ScriptPath: " + ScriptPath + "\n").c_str());
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

    UScriptManager::GetInstance().RegisterTypesToState(Lua);

    // 이 컴포넌트 전용 변수 바인딩
    (*Lua)["actor"] = OwnerActor;
    (*Lua)["self"] = this;

    // 스크립트 로드
    try
    {
        // 파일을 직접 읽어서 실행 (한글 경로 문제 해결)
        std::ifstream file(AbsolutePath, std::ios::binary);
        if (!file.is_open())
        {
            UE_LOG("Failed to open script file\n");
            bScriptLoaded = false;
            return false;
        }

        // 파일 내용 읽기
        std::string scriptContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // 스크립트 실행
        Lua->script(scriptContent);
        bScriptLoaded = true;

        // Store timestamp for hot-reload
        auto Ftime = fs::last_write_time(AbsolutePath);
        LastScriptWriteTime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Ftime.time_since_epoch()).count();

        UE_LOG("Script loaded successfully\n");
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
int UScriptComponent::StartCoroutine(sol::function EntryPoint)
{
	EnsureCoroutineHelper();
	if (CoroutineHelper)
	{
		return CoroutineHelper->StartCoroutine(std::move(EntryPoint));
	}
	return -1;
}

void UScriptComponent::StopCoroutine(int CoroutineID)
{
	if (CoroutineHelper)
	{
		CoroutineHelper->StopCoroutine(CoroutineID);
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
		CoroutineHelper->StopAllCoroutines();
	}
}

void UScriptComponent::EnsureCoroutineHelper()
{
	if (!CoroutineHelper)
	{
		CoroutineHelper = new FCoroutineHelper(this);
	}
}

void UScriptComponent::CheckHotReload(float DeltaTime)
{
    HotReloadCheckTimer += DeltaTime;

    if (HotReloadCheckTimer <= 1.0f) { return; }

    HotReloadCheckTimer = 0.0f;
    if (ScriptPath.empty()) { return; }

    namespace fs = std::filesystem;

    fs::path absolutePath = UScriptManager::ResolveScriptPath(ScriptPath);
    if (fs::exists(absolutePath))
    {
        try
        {
            auto ftime = fs::last_write_time(absolutePath);
            long long currentTime_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(ftime.time_since_epoch()).count();

            if (currentTime_ms > LastScriptWriteTime_ms)
            {
                UE_LOG("Hot-reloading script...\n");
                ReloadScript();
            }
        }
        catch (const fs::filesystem_error& e)
        {
            UE_LOG(e.what());
        }
    }
}

// ==================== Lua Events ====================
void UScriptComponent::NotifyOverlap(AActor* OtherActor)
{
    if (!bScriptLoaded || !OtherActor)
        return;
    
    CallLuaFunction("OnOverlap", OtherActor);
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
	Lua = nullptr;
}

void UScriptComponent::PostDuplicate()
{
	Super::PostDuplicate();

	// Lua state 재생성
	if (!Lua)
	{
		Lua = new sol::state();
	}

	EnsureCoroutineHelper();
}
