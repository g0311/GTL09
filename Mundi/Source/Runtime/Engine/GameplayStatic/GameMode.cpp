#include "pch.h"
#include "GameMode.h"

IMPLEMENT_CLASS(AGameMode)

BEGIN_PROPERTIES(AGameMode)
    // AGameModeBase의 프로퍼티를 상속받으므로 추가 프로퍼티 불필요
END_PROPERTIES()

// ==================== Construction ====================
AGameMode::AGameMode()
{
    // AGameModeBase의 생성자가 모든 초기화를 담당합니다
    Name = "GameMode";
}
