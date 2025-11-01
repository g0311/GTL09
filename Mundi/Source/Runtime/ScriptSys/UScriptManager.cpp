#include "pch.h"
#include "UScriptManager.h"
#include "Actor.h"
#include "ScriptComponent.h"
#include "ScriptUtils.h"

// ==================== 초기화 ====================
IMPLEMENT_CLASS(UScriptManager)

namespace fs = std::filesystem;

void UScriptManager::RegisterTypesToState(sol::state* state)
{
    if (!state) return;

    RegisterCoreTypes(state);

    RegisterActor(state);
    RegisterScriptComponent(state);
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

void UScriptManager::RegisterReflectedClasses(sol::state* state)
{
}

void UScriptManager::RegisterFName(sol::state* state)
{
    // ==================== FName 등록 ====================
    BEGIN_LUA_TYPE(state, FName, "FName", FName(), FName(const char*))
        ADD_LUA_FUNCTION("ToString", &FName::ToString)
        // 문자열 연결을 위한 메타함수
        ADD_LUA_META_FUNCTION(to_string, &FName::ToString)
    END_LUA_TYPE()
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
    END_LUA_TYPE()
}

void UScriptManager::RegisterScriptComponent(sol::state* state)
{
    // ==================== UScriptComponent 등록 ====================
    BEGIN_LUA_TYPE_NO_CTOR(state, UScriptComponent, "ScriptComponent")
        // Coroutine API
        ADD_LUA_FUNCTION("StartCoroutine", &UScriptComponent::StartCoroutine)
        ADD_LUA_FUNCTION("WaitForSeconds", &UScriptComponent::WaitForSeconds)
        ADD_LUA_FUNCTION("StopAllCoroutines", &UScriptComponent::StopAllCoroutines)

        // Owner Actor 접근
        ADD_LUA_FUNCTION("GetOwner", &UScriptComponent::GetOwner)
    END_LUA_TYPE()
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