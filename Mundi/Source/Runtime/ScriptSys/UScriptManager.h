#pragma once

/**
 * Singleton manager for global Lua type bindings used by ScriptComponent instances.
 *
 * InitializeGlobalLuaState() should be called once at engine startup to prepare bindings.
 */

/**
 * Returns the singleton UScriptManager instance.
 *
 * @returns Reference to the single UScriptManager instance.
 */

/**
 * Initialize the global Lua state and register core bindings for engine-wide use.
 */

/**
 * Get the global Lua state managed by this singleton.
 *
 * @returns Pointer to the global sol::state instance, or nullptr if not initialized.
 */

/**
 * Create a new Lua script by copying a template file for the specified scene and actor.
 *
 * @param SceneName Name of the scene the script belongs to.
 * @param ActorName Name of the actor the script is associated with.
 * @param OutScriptPath [out] Relative path of the created script (e.g., "test.lua").
 * @returns `true` if the script was created successfully, `false` otherwise.
 */

/**
 * Open the given script file in the system's default editor, resolving relative paths first.
 *
 * @param ScriptPath Relative path to the script to edit.
 */

/**
 * Register all global and engine bindings into the provided Lua state.
 *
 * @param state Lua state to register types into.
 */

/**
 * Resolve a relative script path to an absolute filesystem path.
 *
 * @param RelativePath Relative script path (for example: "MyScript.lua").
 * @returns Absolute filesystem path (for example: "C:/Project/LuaScripts/MyScript.lua").
 */

/**
 * Convert an FString (UTF-8) to a std::wstring (UTF-16) suitable for Windows filesystem APIs.
 *
 * @param UTF8Str FString containing UTF-8 encoded text.
 * @returns std::wstring encoded in UTF-16 representing the same text.
 */

/**
 * Register core, engine-wide type bindings into the given Lua state.
 *
 * @param state Lua state to register core types into.
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
     * @brief 전역 Lua state 초기화 (엔진 시작 시 호출)
     */
    void InitializeGlobalLuaState();

    /**
     * @brief 전역 Lua state 반환
     */
    sol::state* GetGlobalLuaState() { return GlobalLuaState.get(); }

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

    void RegisterLOG(sol::state* state);
    void RegisterFName(sol::state* state);
    void RegisterVector(sol::state* state);
    void RegisterQuat(sol::state* state);
    void RegisterTransform(sol::state* state);

    // World bindings
    void RegisterWorld(sol::state* state);

    // Actor bindings
    void RegisterActor(sol::state* state);

    // Component bindings
    void RegisterActorComponent(sol::state* state);
    void RegisterSceneComponent(sol::state* state);
    void RegisterStaticMeshComponent(sol::state* state);
    void RegisterBillboardComponent(sol::state* state);
    void RegisterLightComponent(sol::state* state);
    void RegisterDirectionalLightComponent(sol::state* state);
    void RegisterCameraComponent(sol::state* state);
    void RegisterSoundComponent(sol::state* state);
    void RegisterScriptComponent(sol::state* state);
    void RegisterPlayerController(sol::state* state);
    void RegisterPlayerCameraManager(sol::state* state);
    void RegisterGameMode(sol::state* state);
    void RegisterCameraEnums(sol::state* state);

    // Collision components
    void RegisterPrimitiveComponent(sol::state* state);
    void RegisterShapeComponent(sol::state* state);
    void RegisterBoxComponent(sol::state* state);
    void RegisterProjectileMovement(sol::state* state);
    void RegisterRotatingMovement(sol::state* state);

    // Input bindings
    void RegisterInputSubsystem(sol::state* state);
    void RegisterInputContext(sol::state* state);
    void RegisterInputEnums(sol::state* state);

    // ==================== Private Members ====================
    std::unique_ptr<sol::state> GlobalLuaState;  // 전역 Lua state (모든 스크립트 공유)
};