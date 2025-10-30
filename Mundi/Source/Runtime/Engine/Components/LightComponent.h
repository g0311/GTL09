﻿#pragma once
#include "LightComponentBase.h"

class FSceneView;

struct FShadowRenderRequest;


// 실제 조명 효과를 가진 라이트들의 공통 베이스
class ULightComponent : public ULightComponentBase
{
public:
	DECLARE_CLASS(ULightComponent, ULightComponentBase)
	GENERATED_REFLECTION_BODY()

	ULightComponent();
	virtual ~ULightComponent() override;

public:
	// Temperature
	void SetTemperature(float InTemperature) { Temperature = InTemperature;}
	float GetTemperature() const { return Temperature; }

	// 색상과 강도를 합쳐서 반환
	virtual FLinearColor GetLightColorWithIntensity() const;
	void OnRegister(UWorld* InWorld) override;

	virtual void GetShadowRenderRequests(FSceneView* View, TArray<FShadowRenderRequest>& OutRequests) {};

	// Serialization & Duplication
	virtual void OnSerialized() override;
	virtual void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ULightComponent)

	float GetShadowResolutionScale() const { return ShadowResolutionScale; }
	float GetShadowBias() const { return ShadowBias; }
	float GetShadowSlopeBias() const { return ShadowSlopeBias; }
	float GetShadowSharpen() const { return ShadowSharpen; }

protected:
	float Temperature = 6500.0f; // 색온도 (K)

	int ShadowResolutionScale = 1024;
	float ShadowBias = 0.001f;
	float ShadowSlopeBias = 0.01f;
	float ShadowSharpen = 0.2f;

};
