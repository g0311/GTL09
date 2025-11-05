#pragma once
#include "Actor.h"

class UCameraComponent;
class UCameraModifier;
struct FPostProcessSettings;

enum class ECameraBlendType : uint8
{
	Linear,
	/* 필요하다면 추가할 것 */
};

struct FViewTarget
{
	AActor* Target = nullptr; 	//타겟 액터 (카메라) 
	/* 필요하다면 추가할 것(Transform, FOV) */
};

struct FCameraCache
{
	FVector Location;
	FQuat Rotation;
	float FOV;
};

/**
 * @brief 플레이어 카메라를 관리하는 매니저
 *
 * 역할:
 * 1. ViewTarget 관리 (어떤 카메라를 볼 것인가)
 * 2. Fade 효과 (화면 전환)
 * 3. CameraModifier 관리 (화면 흔들림 등)
 *
 * Phase 1 구현 범위:
 * - ViewTarget 즉시 전환 (블렌딩 없음)
 * - 기본 Fade In/Out (선형)
 * - CameraModifier Fade In/Out
 *
 * 중요: PlayerCameraManager는 모든 액터 업데이트 후 Tick됩니다.
 * World::Tick에서 명시적으로 호출되므로 ViewTarget의 최신 위치/회전을 사용합니다.
 */
class APlayerCameraManager : public AActor
{
public:
	DECLARE_CLASS(APlayerCameraManager, AActor)
	GENERATED_REFLECTION_BODY()

	APlayerCameraManager();

protected:
	virtual ~APlayerCameraManager() override;

public:
	void BeginPlay() override;
	void Tick(float DeltaSeconds)  override;
	void EndPlay(EEndPlayReason Reason)  override;

	// Fade In & Out
	void StartFadeOut(float Duration, FLinearColor ToColor = FLinearColor(0, 0, 0, 1));
	void StartFadeIn(float Duration, FLinearColor FromColor = FLinearColor(0, 0, 0, 1));

	UCameraModifier* AddCameraModifier(UCameraModifier* ModifierClass);
	void RemoveCameraModifier(UCameraModifier* Modifier);

	void SetViewTarget(AActor* NewViewTarget, float BlendTime = 0.0f, ECameraBlendType BlendFunc = ECameraBlendType::Linear);
	AActor* GetViewTarget() const { return ViewTarget.Target; }

	UCameraComponent* GetViewTargetCameraComponent() const;

	float GetFadeAmount() const { return FadeAmount; }

	FLinearColor GetFadeColor() const { return FadeColor; }

	/**
	 * @brief 현재 후처리 설정을 반환 (Renderer에 전달용)
	 * @return 모든 CameraModifier가 기여한 후처리 효과 설정
	 */
	FPostProcessSettings GetPostProcessSettings() const;

	FMatrix GetViewMatrix() const;
	
	FMatrix GetProjectionMatrix(FViewport* Viewport) const;
	
	FVector GetCameraLocation() const { return ViewCache.Location; }
	
	FQuat GetCameraRotation() const { return ViewCache.Rotation; }

	// Serialize
	void OnSerialized() override;
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
	void UpdateFade(float DeltaTime);
	void UpdateCamera(float DeltaTime);

	FViewTarget ViewTarget;
	FCameraCache ViewCache;

	TArray<UCameraModifier*> ModifierList;

	// Fade 관련 변수
	FLinearColor FadeColor;           // Fade 색상
	float FadeAmount;                 // 현재 Fade Alpha (0~1)
	float FadeTime;                   // Fade 총 시간
	float FadeTimeRemaining;          // 남은 시간
	bool bIsFading;                   // Fade 중인지
	bool bIsFadingOut;                // true = Out, false = In

};
