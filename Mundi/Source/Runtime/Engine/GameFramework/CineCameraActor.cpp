#include "pch.h"
#include "CineCameraActor.h"
#include "CameraComponent.h"
#include "SceneComponent.h"
#include "ObjectFactory.h"
#include "RenderManager.h"
#include "Renderer.h"

IMPLEMENT_CLASS(ACineCameraActor)

BEGIN_PROPERTIES(ACineCameraActor)
    MARK_AS_SPAWNABLE("시네 카메라", "베지어 곡선 경로를 따라 움직이는 시네마틱 카메라입니다.")
    ADD_PROPERTY_RANGE(float, Duration, "CineCamera", 0.0f, 1000.0f, true, "전체 재생 시간 (초)")
    ADD_PROPERTY_RANGE(float, PlaybackSpeed, "CineCamera", 0.0f, 10.0f, true, "재생 속도 배율")
    ADD_PROPERTY(bool, bLoop, "CineCamera", true, "재생 완료 후 루프")
    ADD_PROPERTY(bool, bShowPath, "CineCamera", true, "경로 시각화 표시")
END_PROPERTIES()

ACineCameraActor::ACineCameraActor()
    : PlaybackTime(0.0f)
    , Duration(10.0f)
    , PlaybackSpeed(1.0f)
    , bPlaying(false)
    , bLoop(false)
    , bShowPath(true)
    , bPathLinesDirty(true)
{
    Name = "CineCameraActor";

    // Root component
    RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

    // Camera component
    CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
    CameraComponent->SetupAttachment(RootComponent);

    bTickInEditor = true;
}

ACineCameraActor::~ACineCameraActor()
{
}

void ACineCameraActor::BeginPlay()
{
    Super::BeginPlay();
}

void ACineCameraActor::CollectPathPointsRecursive(USceneComponent* Parent)
{
    if (!Parent) return;

    const TArray<USceneComponent*>& Children = Parent->GetAttachChildren();
    for (USceneComponent* Child : Children)
    {
        if (!Child) continue;

        // CameraComponent는 제외
        if (Child == CameraComponent) continue;

        // 현재 자식을 PathPoints에 추가
        PathPoints.Add(Child);

        // 재귀적으로 자손들도 추가
        CollectPathPointsRecursive(Child);
    }

    // PathPoints가 변경되었으므로 캐시 무효화
    bPathLinesDirty = true;
}

void ACineCameraActor::RenderDebugVolume(URenderer* Renderer) const
{
    if (!Renderer) return;

    // bShowPath가 false이거나 PathPoints가 부족하면 렌더링하지 않음
    if (!bShowPath || PathPoints.size() < 2)
    {
        return;
    }

    // PathPoints가 변경되었을 때만 재계산
    if (bPathLinesDirty)
    {
        CachedStartPoints.clear();
        CachedEndPoints.clear();
        CachedColors.clear();

        const FVector4 PathColor = FVector4(0.0f, 1.0f, 1.0f, 1.0f); // 청록색
        const FVector4 PointColor = FVector4(1.0f, 0.5f, 0.0f, 1.0f); // 주황색

        // 베지어 곡선 샘플링 (100개 세그먼트)
        const int NumSamples = 100;
        for (int i = 0; i < NumSamples; ++i)
        {
            float t1 = (float)i / (float)NumSamples;
            float t2 = (float)(i + 1) / (float)NumSamples;

            FVector Loc1, Loc2;
            FQuat Rot1, Rot2;
            const_cast<ACineCameraActor*>(this)->EvaluatePath(t1, Loc1, Rot1);
            const_cast<ACineCameraActor*>(this)->EvaluatePath(t2, Loc2, Rot2);

            CachedStartPoints.Add(Loc1);
            CachedEndPoints.Add(Loc2);
            CachedColors.Add(PathColor);
        }

        // PathPoint 위치에 작은 십자가 그리기
        for (USceneComponent* Point : PathPoints)
        {
            if (Point)
            {
                FVector Pos = Point->GetWorldLocation();
                float Size = 2.0f; // 십자가 크기

                // X축
                CachedStartPoints.Add(Pos + FVector(-Size, 0, 0));
                CachedEndPoints.Add(Pos + FVector(Size, 0, 0));
                CachedColors.Add(FVector4(1.f, 0.f, 0.f, 1.f));

                // Y축
                CachedStartPoints.Add(Pos + FVector(0, -Size, 0));
                CachedEndPoints.Add(Pos + FVector(0, Size, 0));
                CachedColors.Add(FVector4(0.f, 1.f, 0.f, 1.f));

                // Z축
                CachedStartPoints.Add(Pos + FVector(0, 0, -Size));
                CachedEndPoints.Add(Pos + FVector(0, 0, Size));
                CachedColors.Add(FVector4(0.f, 0.f, 1.f, 1.f));
            }
        }

        bPathLinesDirty = false;
    }

    // 캐시된 라인 데이터 렌더링
    Renderer->AddLines(CachedStartPoints, CachedEndPoints, CachedColors);
}

void ACineCameraActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 재생 중일 때만 카메라 업데이트
    if (bPlaying && PathPoints.size() >= 2)
    {
        // 시간 업데이트
        PlaybackTime += DeltaSeconds * PlaybackSpeed;

        // 루프 처리
        if (PlaybackTime >= Duration)
        {
            if (bLoop)
            {
                PlaybackTime = fmodf(PlaybackTime, Duration);
            }
            else
            {
                PlaybackTime = Duration;
                bPlaying = false;
            }
        }

        // 정규화된 시간 (0~1)
        float t = Duration > 0.0f ? (PlaybackTime / Duration) : 0.0f;
        t = FMath::Clamp(t, 0.0f, 1.0f);

        // 경로 평가
        FVector Location;
        FQuat Rotation;
        EvaluatePath(t, Location, Rotation);

        // 카메라 업데이트
        if (CameraComponent)
        {
            CameraComponent->SetWorldLocation(Location);
            CameraComponent->SetWorldRotation(Rotation);
        }
    }
}

//================================================
// 경로 포인트 관리
//================================================

void ACineCameraActor::AddPathPoint(USceneComponent* Point)
{
    if (Point)
    {
        PathPoints.Add(Point);
    }
}

void ACineCameraActor::RemovePathPoint(int Index)
{
    if (Index >= 0 && Index < PathPoints.size())
    {
        PathPoints.erase(PathPoints.begin() + Index);
    }
}

void ACineCameraActor::ClearPathPoints()
{
    PathPoints.clear();
}

USceneComponent* ACineCameraActor::GetPathPoint(int Index) const
{
    if (Index >= 0 && Index < PathPoints.size())
    {
        return PathPoints[Index];
    }
    return nullptr;
}

//================================================
// 재생 제어
//================================================

void ACineCameraActor::Play()
{
    // RootComponent의 모든 자손 SceneComponent를 재귀적으로 찾아서 PathPoints에 추가
    if (RootComponent)
    {
        PathPoints.clear();
        CollectPathPointsRecursive(RootComponent);

        if (PathPoints.size() > 0)
        {
            UE_LOG("[CineCameraActor] Found %d path points", PathPoints.size());
        }
    }
    
    bPlaying = true;
}

void ACineCameraActor::Stop()
{
    bPlaying = false;
    PlaybackTime = 0.0f;
}

void ACineCameraActor::Pause()
{
    bPlaying = false;
}

void ACineCameraActor::SetPlaybackTime(float Time)
{
    PlaybackTime = FMath::Clamp(Time, 0.0f, Duration);
}

void ACineCameraActor::SetPlaybackTimeNormalized(float NormalizedTime)
{
    PlaybackTime = FMath::Clamp(NormalizedTime, 0.0f, 1.0f) * Duration;
}

float ACineCameraActor::GetPlaybackTimeNormalized() const
{
    return Duration > 0.0f ? (PlaybackTime / Duration) : 0.0f;
}

//================================================
// 베지어 곡선 계산
//================================================

void ACineCameraActor::EvaluatePath(float t, FVector& OutLocation, FQuat& OutRotation)
{
    const int NumPoints = PathPoints.size();
    if (NumPoints < 2)
    {
        OutLocation = FVector(0, 0, 0);
        OutRotation = FQuat(0, 0, 0, 1);
        return;
    }

    // 단일 세그먼트인 경우
    if (NumPoints == 2)
    {
        USceneComponent* P0 = PathPoints[0];
        USceneComponent* P1 = PathPoints[1];

        if (!P0 || !P1)
        {
            OutLocation = FVector(0, 0, 0);
            OutRotation = FQuat(0, 0, 0, 1);
            return;
        }

        // 단순 선형 보간
        OutLocation = P0->GetWorldLocation() * (1.0f - t) + P1->GetWorldLocation() * t;
        OutRotation = FQuat::Slerp(P0->GetWorldRotation(), P1->GetWorldRotation(), t);
        return;
    }

    // 여러 세그먼트: 전체 경로를 세그먼트로 나눔
    const int NumSegments = NumPoints - 1;
    const float SegmentT = t * NumSegments;
    const int SegmentIndex = FMath::Clamp((int)SegmentT, 0, NumSegments - 1);
    const float LocalT = SegmentT - SegmentIndex;

    // 현재 세그먼트의 포인트들
    USceneComponent* P0 = PathPoints[SegmentIndex];
    USceneComponent* P1 = PathPoints[SegmentIndex + 1];

    if (!P0 || !P1)
    {
        OutLocation = FVector(0, 0, 0);
        OutRotation = FQuat(0, 0, 0, 1);
        return;
    }

    // 컨트롤 포인트 계산
    FVector C0 = CalculateControlPoint(SegmentIndex, false);     // P0에서 나가는 컨트롤
    FVector C1 = CalculateControlPoint(SegmentIndex + 1, true);  // P1로 들어가는 컨트롤

    FQuat Q0_ctrl = CalculateControlRotation(SegmentIndex, false);
    FQuat Q1_ctrl = CalculateControlRotation(SegmentIndex + 1, true);

    // Cubic Bezier 보간
    OutLocation = CubicBezier(P0->GetWorldLocation(), C0, C1, P1->GetWorldLocation(), LocalT);
    OutRotation = CubicBezierRotation(P0->GetWorldRotation(), Q0_ctrl, Q1_ctrl, P1->GetWorldRotation(), LocalT);
}

FVector ACineCameraActor::CubicBezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float mt = 1.0f - t;
    const float mt2 = mt * mt;
    const float mt3 = mt2 * mt;

    return P0 * mt3 + P1 * (3.0f * mt2 * t) + P2 * (3.0f * mt * t2) + P3 * t3;
}

FQuat ACineCameraActor::CubicBezierRotation(const FQuat& Q0, const FQuat& Q1, const FQuat& Q2, const FQuat& Q3, float t)
{
    // De Casteljau's algorithm for quaternion (Slerp 기반)
    FQuat Q01 = FQuat::Slerp(Q0, Q1, t);
    FQuat Q12 = FQuat::Slerp(Q1, Q2, t);
    FQuat Q23 = FQuat::Slerp(Q2, Q3, t);

    FQuat Q012 = FQuat::Slerp(Q01, Q12, t);
    FQuat Q123 = FQuat::Slerp(Q12, Q23, t);

    return FQuat::Slerp(Q012, Q123, t);
}

FVector ACineCameraActor::CalculateControlPoint(int Index, bool bIsFirst)
{
    const int NumPoints = PathPoints.size();
    if (Index < 0 || Index >= NumPoints)
    {
        return FVector(0, 0, 0);
    }

    USceneComponent* P = PathPoints[Index];
    if (!P) return FVector(0, 0, 0);

    FVector Pos = P->GetWorldLocation();

    // Catmull-Rom 스타일 tangent 계산
    if (bIsFirst)
    {
        // 들어오는 컨트롤 포인트 (P로 들어옴)
        if (Index == 0)
        {
            // 첫 포인트: 다음 포인트 방향 사용
            if (NumPoints > 1 && PathPoints[1])
            {
                FVector Next = PathPoints[1]->GetWorldLocation();
                return Pos - (Next - Pos) * 0.333f;
            }
        }
        else if (Index == NumPoints - 1)
        {
            // 마지막 포인트: 이전 포인트 방향 사용
            if (PathPoints[Index - 1])
            {
                FVector Prev = PathPoints[Index - 1]->GetWorldLocation();
                return Pos - (Pos - Prev) * 0.333f;
            }
        }
        else
        {
            // 중간 포인트: tangent 계산
            if (PathPoints[Index - 1] && PathPoints[Index + 1])
            {
                FVector Prev = PathPoints[Index - 1]->GetWorldLocation();
                FVector Next = PathPoints[Index + 1]->GetWorldLocation();
                FVector Tangent = (Next - Prev) * 0.5f;
                return Pos - Tangent * 0.333f;
            }
        }
    }
    else
    {
        // 나가는 컨트롤 포인트 (P에서 나감)
        if (Index == 0)
        {
            // 첫 포인트: 다음 포인트 방향 사용
            if (NumPoints > 1 && PathPoints[1])
            {
                FVector Next = PathPoints[1]->GetWorldLocation();
                return Pos + (Next - Pos) * 0.333f;
            }
        }
        else if (Index == NumPoints - 1)
        {
            // 마지막 포인트: 이전 포인트 방향 사용
            if (PathPoints[Index - 1])
            {
                FVector Prev = PathPoints[Index - 1]->GetWorldLocation();
                return Pos + (Pos - Prev) * 0.333f;
            }
        }
        else
        {
            // 중간 포인트: tangent 계산
            if (PathPoints[Index - 1] && PathPoints[Index + 1])
            {
                FVector Prev = PathPoints[Index - 1]->GetWorldLocation();
                FVector Next = PathPoints[Index + 1]->GetWorldLocation();
                FVector Tangent = (Next - Prev) * 0.5f;
                return Pos + Tangent * 0.333f;
            }
        }
    }

    return Pos;
}

FQuat ACineCameraActor::CalculateControlRotation(int Index, bool bIsFirst)
{
    const int NumPoints = PathPoints.size();
    if (Index < 0 || Index >= NumPoints)
    {
        return FQuat(0, 0, 0, 1);
    }

    USceneComponent* P = PathPoints[Index];
    if (!P) return FQuat(0, 0, 0, 1);

    FQuat Rot = P->GetWorldRotation();

    // 단순화: 인접한 회전 사이를 Slerp
    // 더 정교한 구현이 필요하면 tangent 기반 계산 추가 가능
    if (bIsFirst)
    {
        if (Index > 0 && PathPoints[Index - 1])
        {
            return FQuat::Slerp(PathPoints[Index - 1]->GetWorldRotation(), Rot, 0.667f);
        }
    }
    else
    {
        if (Index < NumPoints - 1 && PathPoints[Index + 1])
        {
            return FQuat::Slerp(Rot, PathPoints[Index + 1]->GetWorldRotation(), 0.333f);
        }
    }

    return Rot;
}

//================================================
// Duplication
//================================================

void ACineCameraActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // 복제된 컴포넌트로 포인터 갱신
    CameraComponent = FindComponentByClass<UCameraComponent>();

    if (RootComponent)
    {
        PathPoints.clear();
        CollectPathPointsRecursive(RootComponent);
    }
}

//================================================
// Serialization
//================================================

void ACineCameraActor::OnSerialized()
{
    Super::OnSerialized();

    // 네이티브 컴포넌트는 직렬화되지 않으므로 매번 다시 찾기
    CameraComponent = FindComponentByClass<UCameraComponent>();
    CollectPathPointsRecursive(RootComponent);
}

void ACineCameraActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // TODO: PathPoints 직렬화 (USceneComponent* 참조 저장/로드)
    // 현재는 런타임에서만 사용
}
