#pragma once
#include "SceneComponent.h"

/**
 * @brief 시네마틱 카메라 경로 포인트
 *
 * CineCameraActor의 베지어 경로를 정의하는 포인트 컴포넌트입니다.
 * 빈 SceneComponent로, 위치와 회전만 정의합니다.
 */
class UPathPointComponent : public USceneComponent
{
public:
    DECLARE_CLASS(UPathPointComponent, USceneComponent)
    GENERATED_REFLECTION_BODY()

    UPathPointComponent();
    virtual ~UPathPointComponent() override;

    // 복사 관련
    virtual void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UPathPointComponent)
};
