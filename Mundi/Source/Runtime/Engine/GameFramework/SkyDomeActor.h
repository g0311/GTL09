#pragma once
#include "Actor.h"

class UStaticMeshComponent;

class ASkyDomeActor : public AActor
{
public:
	DECLARE_CLASS(ASkyDomeActor, AActor)
	GENERATED_REFLECTION_BODY()

	ASkyDomeActor();

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	void UpdatePosition(const FVector& CameraPosition);
	
	// Getter
	UStaticMeshComponent* GetStaticMeshComponent() const { return SkyMeshComponent; }

	// Setter
	void SetSkyTexture(const FString& TexturePath);
	void SetSkyScale(float Scale);

	// Duplicate
	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ASkyDomeActor)

	// Serialize
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	void OnSerialized() override;

protected:
	virtual ~ASkyDomeActor() override;

private:
	void InitializeSkyDome();

	UStaticMeshComponent* SkyMeshComponent;
	float SkyScale;
	FString CurrentTexturePath;
	FString SkyMeshPath;
};
