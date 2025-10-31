﻿#include "pch.h"
#include "EditorEngine.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(0);
#endif

    // @note lua 헤더가 잘 연결되었는지 확인용 코드 
    // @note 프로그램 실행한 뒤 출력란에서 "Lua is running!"이 있는지 확인하면 됩니다.
    // @note 잘 작동한다면 해당 코드를 지우면 됩니다. (25.10.31 PYB)
    //sol::state lua;
    //lua.open_libraries(sol::lib::base, sol::lib::package);

    //lua.set_function("print", [](const std::string& msg) {
    //    OutputDebugStringA((msg + "\n").c_str());
    //    });

    //lua.script("print('Lua is running!')");

    if (!GEngine.Startup(hInstance))
        return -1;

    GEngine.MainLoop();
    GEngine.Shutdown();

    return 0;
}
