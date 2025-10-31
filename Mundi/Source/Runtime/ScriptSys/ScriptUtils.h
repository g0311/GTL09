#pragma once

#define LUA_BIND_CONSTRUCTORS(...) \
    sol::call_constructor, \
    sol::constructors<__VA_ARGS__>()

#define LUA_BIND_MEMBER(MemberName) \
    LuaBindUtils::GetMemberName(#MemberName, true), MemberName

#define LUA_BIND_FUNC(MemberName) LUA_BIND_MEMBER(MemberName)

#define LUA_BIND_STATIC(MemberName) \
    LuaBindUtils::GetMemberName(#MemberName), sol::var(MemberName)

#define LUA_BIND_OVERLOAD_WITHOUT_NAME2(MemberFunc, Type1, Type2)                                           sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc))
#define LUA_BIND_OVERLOAD_WITHOUT_NAME3(MemberFunc, Type1, Type2, Type3)                                    sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc), sol::resolve<Type3>(MemberFunc))
#define LUA_BIND_OVERLOAD_WITHOUT_NAME4(MemberFunc, Type1, Type2, Type3, Type4)                             sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc), sol::resolve<Type3>(MemberFunc), sol::resolve<Type4>(MemberFunc))
#define LUA_BIND_OVERLOAD_WITHOUT_NAME5(MemberFunc, Type1, Type2, Type3, Type4, Type5)                      sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc), sol::resolve<Type3>(MemberFunc), sol::resolve<Type4>(MemberFunc), sol::resolve<Type5>(MemberFunc))
#define LUA_BIND_OVERLOAD_WITHOUT_NAME6(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6)               sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc), sol::resolve<Type3>(MemberFunc), sol::resolve<Type4>(MemberFunc), sol::resolve<Type5>(MemberFunc), sol::resolve<Type6>(MemberFunc))
#define LUA_BIND_OVERLOAD_WITHOUT_NAME7(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6, Type7)        sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc), sol::resolve<Type3>(MemberFunc), sol::resolve<Type4>(MemberFunc), sol::resolve<Type5>(MemberFunc), sol::resolve<Type6>(MemberFunc), sol::resolve<Type7>(MemberFunc))
#define LUA_BIND_OVERLOAD_WITHOUT_NAME8(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8) sol::overload(sol::resolve<Type1>(MemberFunc), sol::resolve<Type2>(MemberFunc), sol::resolve<Type3>(MemberFunc), sol::resolve<Type4>(MemberFunc), sol::resolve<Type5>(MemberFunc), sol::resolve<Type6>(MemberFunc), sol::resolve<Type7>(MemberFunc), sol::resolve<Type8>(MemberFunc))

#define LUA_BIND_OVERLOAD2(MemberFunc, Type1, Type2)                                           LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME2(MemberFunc, Type1, Type2)
#define LUA_BIND_OVERLOAD3(MemberFunc, Type1, Type2, Type3)                                    LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME3(MemberFunc, Type1, Type2, Type3)
#define LUA_BIND_OVERLOAD4(MemberFunc, Type1, Type2, Type3, Type4)                             LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME4(MemberFunc, Type1, Type2, Type3, Type4)
#define LUA_BIND_OVERLOAD5(MemberFunc, Type1, Type2, Type3, Type4, Type5)                      LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME5(MemberFunc, Type1, Type2, Type3, Type4, Type5)
#define LUA_BIND_OVERLOAD6(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6)               LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME6(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6)
#define LUA_BIND_OVERLOAD7(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6, Type7)        LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME7(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6, Type7)
#define LUA_BIND_OVERLOAD8(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8) LuaBindUtils::GetMemberName(#MemberFunc), LUA_BIND_OVERLOAD_WITHOUT_NAME8(MemberFunc, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8)
