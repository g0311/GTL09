#include "pch.h"
#include "ScriptComponent.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "ImGui/imgui.h"
#include "Source/Runtime/Core/Game/CoroutineHelper.h"
#include <filesystem>

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
	EnsureCoroutineHelper();
}

UScriptComponent::~UScriptComponent()
{
	StopAllCoroutines();
	delete CoroutineHelper;
	CoroutineHelper = nullptr;
}

// ==================== Lifecycle ====================
void UScriptComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	EnsureCoroutineHelper();

	if (!bScriptLoaded && !ScriptPath.empty())
	{
		ReloadScript();
	}

	if (bScriptLoaded)
	{
		sol::state& lua = UScriptManager::GetInstance().GetLuaState();
		sol::protected_function func = lua["BeginPlay"];
		if (func.valid())
		{
			sol::protected_function_result result = func();
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
	{
		if (CoroutineHelper)
		{
			CoroutineHelper->RunScheduler(DeltaTime);
		}
		return;
	}

	sol::state& lua = UScriptManager::GetInstance().GetLuaState();

	sol::protected_function func = lua["Tick"];
	if (func.valid())
	{
		sol::protected_function_result result = func(DeltaTime);
		if (!result.valid())
		{
			sol::error err = result;
			OutputDebugStringA(("Tick error: " + std::string(err.what()) + "\n").c_str());
		}
	}

	if (CoroutineHelper)
	{
		CoroutineHelper->RunScheduler(DeltaTime);
	}
}

void UScriptComponent::EndPlay(EEndPlayReason Reason)
{
	if (bScriptLoaded)
	{
		sol::state& lua = UScriptManager::GetInstance().GetLuaState();
		sol::protected_function func = lua["EndPlay"];
		if (func.valid())
		{
			sol::protected_function_result result = func();
			if (!result.valid())
			{
				sol::error err = result;
				OutputDebugStringA(("EndPlay error: " + std::string(err.what()) + "\n").c_str());
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
		OutputDebugStringA("No script path set\n");
		return;
	}

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

	if (!fs::exists(absolutePath))
	{
		FString errorMsg = "Script file not found: " + absolutePath.string();
		MessageBoxA(NULL, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
		return;
	}

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
	if (ScriptPath.empty()) { return false; }

	namespace fs = std::filesystem;
	if (!fs::exists(ScriptPath))
	{
		OutputDebugStringA(("Script file not found: " + ScriptPath + "\n").c_str());
		bScriptLoaded = false;
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		OutputDebugStringA("No owner actor\n");
		return false;
	}

	StopAllCoroutines();

	sol::state& lua = UScriptManager::GetInstance().GetLuaState();
	lua["actor"] = OwnerActor;
	lua["self"] = this;

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
	{
		return;
	}

	sol::state& lua = UScriptManager::GetInstance().GetLuaState();
	sol::protected_function func = lua["OnOverlap"];
	if (func.valid())
	{
		sol::protected_function_result result = func(OtherActor);
		if (!result.valid())
		{
			sol::error err = result;
			OutputDebugStringA(("OnOverlap error: " + std::string(err.what()) + "\n").c_str());
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

	bScriptLoaded = false;
	CoroutineHelper = nullptr;
}

void UScriptComponent::PostDuplicate()
{
	Super::PostDuplicate();
	EnsureCoroutineHelper();
}
