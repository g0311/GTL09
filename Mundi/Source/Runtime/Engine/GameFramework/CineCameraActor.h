#pragma once
#include "Actor.h"

class UCameraComponent;
class USceneComponent;

/**
 * @brief 시네마틱 카메라 액터
 *
 * SceneComponent 배열(PathPoints)을 따라 베지어 곡선으로 카메라를 이동시킵니다.
 * 각 PathPoint의 Transform을 사용하여 부드러운 경로를 생성합니다.
 */
class ACineCameraActor : public AActor
{
public:
    DECLARE_CLASS(ACineCameraActor, AActor)
    GENERATED_REFLECTION_BODY()

    ACineCameraActor();
    virtual ~ACineCameraActor() override;

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void OnPossessed() override;

    // Debug rendering
    void RenderDebugVolume(class URenderer* Renderer) const;

    //================================================
    // 경로 포인트 관리
    //================================================

    /** PathPoint 추가 */
    void AddPathPoint(USceneComponent* Point);

    /** PathPoint 제거 */
    void RemovePathPoint(int Index);

    /** 모든 PathPoint 제거 */
    void ClearPathPoints();

    /** PathPoint 개수 반환 */
    int GetPathPointCount() const { return PathPoints.size(); }

    /** 특정 인덱스의 PathPoint 반환 */
    USceneComponent* GetPathPoint(int Index) const;

    //================================================
    // 재생 제어
    //================================================

    /** 재생 시작 */
    void Play();

    /** 재생 정지 */
    void Stop();

    /** 재생 일시정지 */
    void Pause();

    /** 특정 시간으로 이동 (0.0 ~ Duration) */
    void SetPlaybackTime(float Time);

    /** 정규화된 시간으로 이동 (0.0 ~ 1.0) */
    void SetPlaybackTimeNormalized(float NormalizedTime);

    /** 재생 중인지 여부 */
    bool IsPlaying() const { return bPlaying; }

    //================================================
    // 설정
    //================================================

    /** 전체 재생 시간 설정 (초) */
    void SetDuration(float InDuration) { Duration = InDuration; }
    float GetDuration() const { return Duration; }

    /** 재생 속도 설정 (1.0 = 기본) */
    void SetPlaybackSpeed(float Speed) { PlaybackSpeed = Speed; }
    float GetPlaybackSpeed() const { return PlaybackSpeed; }

    /** 루프 여부 설정 */
    void SetLoop(bool bInLoop) { bLoop = bInLoop; }
    bool IsLooping() const { return bLoop; }

    /** 경로 시각화 표시 설정 */
    void SetShowPath(bool bInShow) { bShowPath = bInShow; bPathLinesDirty = true; }
    bool IsShowingPath() const { return bShowPath; }

    /** 현재 재생 시간 반환 */
    float GetPlaybackTime() const { return PlaybackTime; }

    /** 정규화된 재생 시간 반환 (0.0 ~ 1.0) */
    float GetPlaybackTimeNormalized() const;

    /** 자동 전환 설정 */
    void SetAutoReturnToPlayer(bool bInAuto) { bAutoReturnToPlayer = bInAuto; }
    bool IsAutoReturnToPlayer() const { return bAutoReturnToPlayer; }

    void SetTargetPawnActor(AActor* InActor) { TargetPawnActor = InActor; }
    AActor* GetTargetPawnActor() const { return TargetPawnActor; }

    //================================================
    // 컴포넌트 접근
    //================================================

    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

    // Serialization
    virtual void OnSerialized() override;
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    // Duplication
    virtual void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(ACineCameraActor)

private:
    //================================================
    // 베지어 곡선 계산
    //================================================

    /** RootComponent의 자손들을 재귀적으로 수집하여 PathPoints에 추가 */
    void CollectPathPointsRecursive(USceneComponent* Parent);

    /** t (0~1) 값에 따른 위치와 회전 계산 */
    void EvaluatePath(float t, FVector& OutLocation, FQuat& OutRotation);

    /** Cubic Bezier 보간 (4개 포인트) */
    FVector CubicBezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float t);
    FQuat CubicBezierRotation(const FQuat& Q0, const FQuat& Q1, const FQuat& Q2, const FQuat& Q3, float t);

    /** 인접 포인트들을 이용해 컨트롤 포인트 자동 계산 */
    FVector CalculateControlPoint(int Index, bool bIsFirst);
    FQuat CalculateControlRotation(int Index, bool bIsFirst);

    //================================================
    // 컴포넌트
    //================================================

    UCameraComponent* CameraComponent;

    //================================================
    // 경로 데이터
    //================================================

    TArray<USceneComponent*> PathPoints;

    //================================================
    // 재생 상태
    //================================================

    float PlaybackTime;      // 현재 재생 시간 (초)
    float Duration;          // 전체 재생 시간 (초)
    float PlaybackSpeed;     // 재생 속도 배율
    bool bPlaying;           // 재생 중인지
    bool bLoop;              // 루프 여부
    bool bShowPath;          // 경로 시각화 표시 여부

    //================================================
    // 자동 전환 설정
    //================================================

    bool bAutoReturnToPlayer;    // 재생 완료 시 자동으로 플레이어 Pawn으로 전환
    AActor* TargetPawnActor;     // 재생 완료 후 빙의할 타겟 Pawn (nullptr이면 자동 전환 안함)

    // 경로 라인 캐시 (RenderDebugVolume 성능 최적화)
    mutable bool bPathLinesDirty;
    mutable TArray<FVector> CachedStartPoints;
    mutable TArray<FVector> CachedEndPoints;
    mutable TArray<FVector4> CachedColors;

    /** 직렬화 임시 변수 (OnSerialized에서 TargetPawnActor 복원) */
    FString TargetPawnActorNameToRestore;
};
