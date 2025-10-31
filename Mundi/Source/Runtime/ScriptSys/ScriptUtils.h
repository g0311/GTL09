#pragma once
#include "sol.hpp"

/*
 * ===================================================================
 * ★ "중앙 집중식" 바인딩 함수 (e.g., UScriptManager.cpp) 내부용 ★
 *
 * sol2의 "순차적 할당" 문법(usertype[...] = ...)을 사용해
 * 바인딩 코드의 가독성을 높입니다.
 * ===================================================================
 */

/**
 * @brief Usertype 정의를 시작 (생성자 없음)
 * 'auto usertype = ...' 변수를 생성합니다.
 */
#define BEGIN_LUA_TYPE_NO_CTOR(state_ptr, ClassType, LuaName) \
auto usertype = state_ptr->new_usertype<ClassType>(LuaName);

/**
 * @brief Usertype 정의를 시작 (생성자 포함)
 * 가변 인자(...)로 생성자 목록을 받습니다.
 */
#define BEGIN_LUA_TYPE(state_ptr, ClassType, LuaName, ...) \
auto usertype = state_ptr->new_usertype<ClassType>(LuaName, \
sol::constructors<__VA_ARGS__>() \
);

/**
 * @brief (블록 내부) 함수 등록
 */
#define ADD_LUA_FUNCTION(LuaName, ClassFunctionPtr) \
usertype[LuaName] = ClassFunctionPtr;

/**
 * @brief (블록 내부) 프로퍼티(멤버 변수) 등록
 */
#define ADD_LUA_PROPERTY(LuaName, ClassPropertyPtr) \
usertype[LuaName] = ClassPropertyPtr;

/**
 * @brief (블록 내부) 오버로드(Overload) 함수 등록
 * @param ... (함수 포인터 목록)
 */
#define ADD_LUA_OVERLOAD(LuaName, ...) \
usertype[LuaName] = sol::overload(__VA_ARGS__);

/**
 * @brief (블록 내부) 메타 함수 (연산자) 등록
 */
#define ADD_LUA_META_FUNCTION(MetaFunc, Lambda) \
usertype[sol::meta_function::MetaFunc] = Lambda;

/**
 * @brief (블록 내부) 타입 정의 종료 (가독성용)
 */
#define END_LUA_TYPE() \
(void)0; // 매크로 끝에 세미콜론을 붙일 수 있도록

/**
 * @brief (UObject용) Lua 바인딩 브릿지 함수("inline static")를 "정의 시작"
 * ThisClass_t는 DECLARE_CLASS 매크로가 정의
 */
#define DEFINE_LUA_BINDING_REFLECTED(SuperClass) \
public: \
    inline static void _Internal_StaticLuaBind(void* InLuaState) \
    { \
        sol::state& lua = *static_cast<sol::state*>(InLuaState); \
        /* UClass에서 런타임 이름을 가져옴 */ \
        auto usertype = lua.new_usertype<ThisClass_t>(ThisClass_t::StaticClass()->Name, \
            sol::bases<SuperClass>() /* 부모 클래스 등록 */ \
        ); \
        (void)usertype; // usertype이 사용되지 않을 경우의 경고 방지

/**
 * @brief (UObject용) (블록 내부) 함수 등록
 */
#define BIND_LUA_FUNCTION(LuaName, ClassFunctionPtr) \
    usertype[LuaName] = ClassFunctionPtr;

/**
 * @brief (UObject용) (블록 내부) 프로퍼티(멤버 변수) 등록
 */
#define BIND_LUA_PROPERTY(LuaName, ClassPropertyPtr) \
    usertype[LuaName] = ClassPropertyPtr;
    
/**
 * @brief (UObject용) (블록 내부) 오버로드(Overload) 함수 등록
 */
#define BIND_LUA_OVERLOAD(LuaName, ...) \
    usertype[LuaName] = sol::overload(__VA_ARGS__);

/**
 * @brief (UObject용) Lua 바인딩 블록 "종료"
 */
#define END_LUA_BINDING_REFLECTED() \
    } /* _Internal_StaticLuaBind 함수의 끝 */ \
private: /* 다시 private으로 돌려놓음 */
