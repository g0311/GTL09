#pragma once

/**
 * @class UScriptManager
 * @brief 전역 Lua 타입 바인딩을 관리하는 싱글톤 매니저
 * 
 * 엔진 시작 시 한 번만 InitializeGlobalBindings()를 호출하면
 * 모든 ScriptComponent가 해당 타입 바인딩을 사용할 수 있습니다.
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
     * @brief 전역 Lua state 반환
     */
    sol::state& GetLuaState() { return lua; }
    
    /**
     * @brief template.lua를 복사하여 새 스크립트 파일 생성
     * @param SceneName 씬 이름
     * @param ActorName 액터 이름
     * @return 생성 성공 여부
     */
    bool CreateScript(const FString& SceneName, const FString& ActorName);
    
    /**
     * @brief 스크립트 파일을 기본 에디터로 열기
     * @param ScriptPath 편집할 스크립트 경로
     */
    void EditScript(const FString& ScriptPath);
    
    /**
     * @brief 특정 lua state에 모든 타입 등록 (컴포넌트별 state용)
     */
    void RegisterTypesToState(sol::state* state);
    
    /**
     * @brief 리플렉션 기반 클래스 자동 등록
     */
    void RegisterClassesWithReflection();

private:
    /**
     * @brief 전역 타입 바인딩 등록
     */
    void RegisterGlobalTypes(sol::state* state);
};
