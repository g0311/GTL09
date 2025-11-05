#include "pch.h"
#include "PathPointComponent.h"
#include "Renderer.h"

IMPLEMENT_CLASS(UPathPointComponent)

BEGIN_PROPERTIES(UPathPointComponent)
    MARK_AS_COMPONENT("경로 포인트", "시네마틱 카메라 경로의 제어점입니다.")
END_PROPERTIES()

UPathPointComponent::UPathPointComponent()
{
    bIsEditable = true; // 에디터에서 추가/편집 가능
}

UPathPointComponent::~UPathPointComponent()
{
}

void UPathPointComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
    // PathPointComponent는 추가 멤버 변수가 없으므로 부모의 구현으로 충분
}