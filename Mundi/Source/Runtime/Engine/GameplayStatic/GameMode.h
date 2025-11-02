#pragma once
#include "Source/Runtime/Engine/GameFramework/GameModeBase.h"

/**
 * @class AGameMode
 * @brief AGameModeBase를 상속하는 확장 가능한 GameMode 클래스
 *
 * 모든 핵심 기능은 AGameModeBase에 구현되어 있습니다.
 * 이 클래스는 프로젝트별 커스텀 게임 로직을 추가하기 위한 확장 포인트입니다.
 */
class AGameMode : public AGameModeBase
{
public:
    DECLARE_CLASS(AGameMode, AGameModeBase)
    GENERATED_REFLECTION_BODY()

    AGameMode();
    ~AGameMode() override = default;

    // 추가 커스텀 기능이 필요한 경우 여기에 추가하세요
};
