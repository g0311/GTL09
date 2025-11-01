﻿#pragma once

/**
 * @brief 전역 Lua 타입 바인딩을 관리하는 싱글톤 매니저
 * @note 엔진 시작 시 최초 1회 InitializeGlobalBindings()를 호출하면 
 *       모든 ScriptComponent가 해당 타입 바인딩을 사용할 수 있습니다.
 */
class UScriptManager : public UObject
{
public:
    DECLARE_CLASS(UScriptManager, UObject)

    UScriptManager() = default;

    /**
     * @brief 싱글톤 인스턴스 반환
     */
    static UScriptManager& GetInstance()
    {
        static UScriptManager instance;
        return instance;
    }

    UScriptManager(const UScriptManager&) = delete;
    UScriptManager& operator=(const UScriptManager&) = delete;

    /**
     * @brief template.lua를 복사하여 새 스크립트 파일 생성
     * @param SceneName 씬 이름
     * @param ActorName 액터 이름
     * @param OutScriptPath [out] 생성된 스크립트 파일의 상대 경로 (예: "test.lua")
     * @return 생성 성공 여부
     */
    bool CreateScript(const FString& SceneName, const FString& ActorName, FString& OutScriptPath);

    /**
     * @brief 스크립트 파일을 기본 에디터로 열기 (경로 해석 포함)
     * @param ScriptPath 편집할 스크립트 경로 (상대 경로)
     */
    void EditScript(const FString& ScriptPath);

    /**
     * @brief 특정 lua state에 모든 타입 등록 (컴포넌트별 state용)
     */
    void RegisterTypesToState(sol::state* state);

    // ==================== 파일 시스템 유틸리티 ====================
    /**
     * @brief 상대 스크립트 경로를 절대 경로(std::filesystem::path)로 변환
     * @param RelativePath (e.g., "MyScript.lua")
     * @return (e.g., "C:/Project/LuaScripts/MyScript.lua")
     */
    static std::filesystem::path ResolveScriptPath(const FString& RelativePath);

    /**
     * @brief FString(UTF-8)을 std::wstring(UTF-16)으로 변환 (Windows 파일 시스템 API용)
     * @param UTF8Str 변환할 FString
     * @return UTF-16 인코딩된 std::wstring
     */
    static std::wstring FStringToWideString(const FString& UTF8Str);

private:
    /**
     * @brief 전역 타입 바인딩 등록
     */
    void RegisterCoreTypes(sol::state* state);
    void RegisterReflectedClasses(sol::state* state);

    void RegisterLOG(sol::state* state);
    void RegisterFName(sol::state* state);
    void RegisterVector(sol::state* state);
    void RegisterQuat(sol::state* state);
    void RegisterTransform(sol::state* state);
    void RegisterActor(sol::state* state);
    void RegisterScriptComponent(sol::state* state);
    // void RegisterMovement(sol::state* state);
    void RegisterProjectileMovement(sol::state* state);

    // Input bindings
    void RegisterInputSubsystem(sol::state* state);
    void RegisterInputContext(sol::state* state);
    void RegisterInputEnums(sol::state* state);
};
