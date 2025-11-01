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

