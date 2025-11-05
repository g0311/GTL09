#include "pch.h"
#include "ScriptComponent.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "Source/Runtime/Core/Game/CoroutineHelper.h"
#include "Source/Runtime/Engine/Components/PrimitiveComponent.h"

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

	// 전역 Lua state 사용 (UScriptManager에서 관리)
	// 각 스크립트는 ReloadScript에서 독립 환경 생성

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

    // Lua state는 전역 UScriptManager에서 관리하므로 여기서 삭제하지 않음
}

// ==================== Lifecycle ====================
void UScriptComponent::BeginPlay()
{
	UActorComponent::BeginPlay();
	EnsureCoroutineHelper();

    UE_LOG(("[ScriptComponent] BeginPlay called for: " + GetOwner()->GetName().ToString() + "\n").c_str());
    UE_LOG(("  ScriptPath: '" + ScriptPath + "'\n").c_str());
    UE_LOG(("  bScriptLoaded: " + std::string(bScriptLoaded ? "true" : "false") + "\n").c_str());

    // 스크립트 로드
    if (!bScriptLoaded && !ScriptPath.empty())
    {
        UE_LOG("  Attempting to reload script...\n");
        bool loadResult = ReloadScript();
        UE_LOG(("  ReloadScript() result: " + std::string(loadResult ? "SUCCESS" : "FAILED") + "\n").c_str());
    }
    else
    {
        if (bScriptLoaded)
        {
            UE_LOG("  Script already loaded, skipping reload\n");
        }
        else if (ScriptPath.empty())
        {
            UE_LOG("  ScriptPath is empty, skipping reload\n");
        }
    }
    
	// Lua BeginPlay() 호출
	if (bScriptLoaded)
	{
        UE_LOG("  Calling Lua BeginPlay()...\n");
		CallLuaFunction("BeginPlay");
        UE_LOG("  Lua BeginPlay() finished\n");
	}
    else
    {
        UE_LOG("  WARNING: Script not loaded, skipping Lua BeginPlay()\n");
    }

	// Bind overlap delegates on owner's primitive components to forward into Lua
	if (AActor* Owner = GetOwner())
	{
		int boundCount = 0;
		for (UActorComponent* Comp : Owner->GetOwnedComponents())
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
			{
				Prim->AddOnBeginOverlapDynamic(this, &UScriptComponent::OnBeginOverlap);
				Prim->AddOnEndOverlapDynamic(this, &UScriptComponent::OnEndOverlap);
				boundCount++;
			}
		}
		FString ownerName = Owner->Name.ToString();
		FString msg = FString("[ScriptComponent] BeginPlay: Bound overlap to ") + FString(std::to_string(boundCount)) + " primitive components on " + ownerName + "\n";
		UE_LOG(msg.c_str());
	}
}

void UScriptComponent::TickComponent(float DeltaTime)
{
	UActorComponent::TickComponent(DeltaTime);

    static int tickCount = 0;
    if (tickCount < 5) // 처음 5번만 로그
    {
        UE_LOG(("[ScriptComponent::Tick] " + std::to_string(tickCount) + " Owner: " + (GetOwner() ? GetOwner()->Name.ToString() : "NULL") + ", ScriptPath: " + ScriptPath + "\n").c_str());
        tickCount++;
    }

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

    // 현재 작업 디렉토리 출력
    fs::path currentPath = fs::current_path();
    UE_LOG(("[ScriptComponent] Current working directory: " + currentPath.string() + "\n").c_str());

    fs::path AbsolutePath = UScriptManager::ResolveScriptPath(ScriptPath);

    // 해석된 절대 경로 출력
    UE_LOG(("[ScriptComponent] Resolved absolute path: " + AbsolutePath.string() + "\n").c_str());

    if (!fs::exists(AbsolutePath))
    {
        UE_LOG("[ScriptComponent] ERROR: Script file not found!\n");
        UE_LOG(("  Input ScriptPath: '" + ScriptPath + "'\n").c_str());
        UE_LOG(("  Resolved path: '" + AbsolutePath.string() + "'\n").c_str());
        UE_LOG(("  File exists: false\n"));

        // LuaScripts 폴더 존재 여부 확인
        fs::path luaScriptsDir = currentPath / L"LuaScripts";
        UE_LOG(("  LuaScripts directory: '" + luaScriptsDir.string() + "'\n").c_str());
        UE_LOG(("  LuaScripts exists: " + std::string(fs::exists(luaScriptsDir) ? "true" : "false") + "\n").c_str());

        bScriptLoaded = false;
        return false;
    }

    UE_LOG(("[ScriptComponent] Script file found at: " + AbsolutePath.string() + "\n").c_str());

    // Owner Actor를 Lua에 바인딩
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        UE_LOG("No owner actor\n");
        return false;
    }

    StopAllCoroutines();

    // 전역 Lua state 가져오기
    sol::state* GlobalLua = SCRIPT.GetGlobalLuaState();
    if (!GlobalLua)
    {
        UE_LOG("[ScriptComponent] ERROR: Global Lua state not initialized!\n");
        bScriptLoaded = false;
        return false;
    }

    // 스크립트별 독립 환경 생성 (전역 환경을 fallback으로 사용)
    ScriptEnv = sol::environment(*GlobalLua, sol::create, GlobalLua->globals());

    // 이 컴포넌트 전용 변수 바인딩 (독립 환경에)
    ScriptEnv["actor"] = OwnerActor;
    ScriptEnv["self"] = this;

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

        // 스크립트를 로드 (아직 실행 안 함)
        auto loadResult = GlobalLua->load(scriptContent, ScriptPath);
        if (!loadResult.valid())
        {
            sol::error err = loadResult;
            UE_LOG(("Lua script load error: " + std::string(err.what()) + "\n").c_str());
            bScriptLoaded = false;
            return false;
        }

        // 로드된 chunk 함수에 환경 설정
        sol::protected_function scriptFunc = loadResult;
        sol::set_environment(ScriptEnv, scriptFunc);

        // 스크립트 실행
        auto execResult = scriptFunc();
        if (!execResult.valid())
        {
            sol::error err = execResult;
            UE_LOG(("Lua script execution error: " + std::string(err.what()) + "\n").c_str());
            bScriptLoaded = false;
            return false;
        }

        // 스크립트 환경을 ScriptTable에 저장
        ScriptTable = ScriptEnv;
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
void UScriptComponent::NotifyOverlap(AActor* OtherActor, const FContactInfo& ContactInfo)
{
    if (!bScriptLoaded)
    {
        UE_LOG("[Overlap] Script not loaded!\n");
        return;
    }

    if (!OtherActor)
    {
        UE_LOG("[Overlap] OtherActor is null!\n");
        return;
    }

    if (!ScriptTable.valid())
    {
        UE_LOG("[Overlap] ScriptTable is invalid!\n");
        return;
    }

    UE_LOG("[Overlap] Calling Lua OnOverlap function...\n");

    // FContactInfo를 Lua 테이블로 변환
    sol::state* GlobalLua = SCRIPT.GetGlobalLuaState();
    if (GlobalLua)
    {
        sol::table contactInfoTable = GlobalLua->create_table();
        contactInfoTable["ContactPoint"] = ContactInfo.ContactPoint;
        contactInfoTable["ContactNormal"] = ContactInfo.ContactNormal;
        contactInfoTable["PenetrationDepth"] = ContactInfo.PenetrationDepth;

        CallLuaFunction("OnOverlap", OtherActor, contactInfoTable);
    }
    else
    {
        // Fallback: 충돌 정보 없이 호출
        CallLuaFunction("OnOverlap", OtherActor);
    }
}

void UScriptComponent::OnBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, const FContactInfo& ContactInfo)
{
	// Debug: 충돌 감지 확인
	if (OtherActor && GetOwner())
	{
		FString ownerName = GetOwner()->Name.ToString();
		FString otherName = OtherActor->Name.ToString();
		FString msg = FString("[Overlap] ") + ownerName + " <-> " + otherName + "\n";
		UE_LOG(msg.c_str());
	}

	// Forward to Lua handler
	NotifyOverlap(OtherActor, ContactInfo);
}

void UScriptComponent::OnEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, const FContactInfo& ContactInfo)
{
	// Optional: call Lua if function exists
	if (!bScriptLoaded || !OtherActor)
	{
		return;
	}

	// FContactInfo를 Lua 테이블로 변환 (OnEndOverlap도 정보 제공)
	sol::state* GlobalLua = SCRIPT.GetGlobalLuaState();
	if (GlobalLua)
	{
		sol::table contactInfoTable = GlobalLua->create_table();
		contactInfoTable["ContactPoint"] = ContactInfo.ContactPoint;
		contactInfoTable["ContactNormal"] = ContactInfo.ContactNormal;
		contactInfoTable["PenetrationDepth"] = ContactInfo.PenetrationDepth;

		CallLuaFunction("OnEndOverlap", OtherActor, contactInfoTable);
	}
	else
	{
		CallLuaFunction("OnEndOverlap", OtherActor);
	}
}

// ==================== HUD Bridge ====================
bool UScriptComponent::GetHUDEntries(TArray<FHUDRow>& OutRows)
{
    OutRows.Empty();
    if (!bScriptLoaded || !ScriptTable.valid())
    {
        return false;
    }
    try
    {
        sol::protected_function func = ScriptTable["HUD_GetEntries"];
        if (!func.valid())
        {
            return false;
        }
        sol::protected_function_result result = func();
        if (!result.valid())
        {
            sol::error err = result;
            UE_LOG((FString("[Lua Error] HUD_GetEntries: ") + err.what() + "\n").c_str());
            return false;
        }
        sol::object obj = result;
        if (!obj.is<sol::table>())
        {
            return false;
        }
        sol::table rows = obj.as<sol::table>();
        int n = static_cast<int>(rows.size());
        for (int i = 1; i <= n; ++i)
        {
            sol::object rowObj = rows[i];
            if (!rowObj.is<sol::table>()) continue;
            sol::table row = rowObj.as<sol::table>();
            FHUDRow r;
            sol::optional<std::string> lbl = row["label"];
            sol::optional<std::string> val = row["value"];
            if (lbl) r.Label = *lbl; else r.Label = "";
            if (val) r.Value = *val; else r.Value = "";
            sol::object col = row["color"];
            if (col.valid() && col.is<sol::table>())
            {
                sol::table t = col.as<sol::table>();
                r.bHasColor = true;
                r.R = t.get_or(1, 1.0f);
                r.G = t.get_or(2, 1.0f);
                r.B = t.get_or(3, 1.0f);
                r.A = t.get_or(4, 1.0f);
            }
            OutRows.Add(r);
        }
        return !OutRows.IsEmpty();
    }
    catch (const sol::error& e)
    {
        UE_LOG((FString("[Lua Error] HUD_GetEntries: ") + e.what() + "\n").c_str());
        return false;
    }
}

bool UScriptComponent::GetHUDGameOver(FString& OutTitle, TArray<FString>& OutLines)
{
    OutLines.Empty();
    OutTitle = "";
    if (!bScriptLoaded || !ScriptTable.valid())
    {
        return false;
    }
    try
    {
        sol::protected_function func = ScriptTable["HUD_GameOver"];
        if (!func.valid())
        {
            return false;
        }
        sol::protected_function_result result = func();
        if (!result.valid())
        {
            sol::error err = result;
            UE_LOG((FString("[Lua Error] HUD_GameOver: ") + err.what() + "\n").c_str());
            return false;
        }
        sol::object obj = result;
        if (!obj.is<sol::table>())
        {
            return false;
        }
        sol::table t = obj.as<sol::table>();
        sol::optional<std::string> title = t["title"];
        if (title) OutTitle = *title; else OutTitle = "Game Over";
        sol::object lines = t["lines"];
        if (lines.valid() && lines.is<sol::table>())
        {
            sol::table arr = lines.as<sol::table>();
            int n = static_cast<int>(arr.size());
            for (int i = 1; i <= n; ++i)
            {
                sol::optional<std::string> s = arr[i];
                if (s) OutLines.Add(*s);
            }
        }
        return true;
    }
    catch (const sol::error& e)
    {
        UE_LOG((FString("[Lua Error] HUD_GameOver: ") + e.what() + "\n").c_str());
        return false;
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
	// 전역 Lua state 사용하므로 초기화 불필요
	// ScriptEnv와 ScriptTable은 ReloadScript에서 생성됨
}

void UScriptComponent::PostDuplicate()
{
	Super::PostDuplicate();

	// 전역 Lua state 사용하므로 별도 생성 불필요
	// 스크립트 환경은 ReloadScript에서 생성됨

	EnsureCoroutineHelper();
}
