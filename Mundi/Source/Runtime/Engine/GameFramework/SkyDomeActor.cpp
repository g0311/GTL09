#include "pch.h"
#include "SkyDomeActor.h"
#include "StaticMeshComponent.h"
#include "Material.h"
#include "ResourceManager.h"
#include "World.h"
#include "CameraActor.h"
#include "Texture.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(ASkyDomeActor)

BEGIN_PROPERTIES(ASkyDomeActor)
	MARK_AS_SPAWNABLE("스카이 돔", "하늘을 렌더링하는 대형 돔 액터입니다.")
END_PROPERTIES()

ASkyDomeActor::ASkyDomeActor()
	: SkyScale(10.0f)
{
	Name = "Sky Dome Actor";
	SkyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("SkyMeshComponent");
	RootComponent = SkyMeshComponent;

	// 기본 텍스처 및 메시 경로 설정
	CurrentTexturePath = "Data/Model/Sky/night_2.png";
	SkyMeshPath = "Data/Model/Sky/Sky Dome.obj";

	InitializeSkyDome();
}

ASkyDomeActor::~ASkyDomeActor()
{

}

void ASkyDomeActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASkyDomeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (UWorld* World = GetWorld())
	{
		if (ACameraActor* Camera = World->GetActiveCamera())
		{
			UpdatePosition(Camera->GetActorLocation());
		}
	}
}

void ASkyDomeActor::UpdatePosition(const FVector& CameraPosition)
{
	FVector NewPosition = CameraPosition;
	NewPosition.Z = 0.0f;
	SetActorLocation(NewPosition);
}

void ASkyDomeActor::SetSkyTexture(const FString& TexturePath)
{
	if (TexturePath.empty() || !SkyMeshComponent) { return; }

	if (UMaterialInstanceDynamic* MID = SkyMeshComponent->CreateAndSetMaterialInstanceDynamic(0))
	{
		if (UTexture* Texture = UResourceManager::GetInstance().Load<UTexture>(TexturePath))
		{
			MID->SetTextureParameterValue(EMaterialTextureSlot::Diffuse, Texture);
			CurrentTexturePath = TexturePath; 
		}
	}
}

void ASkyDomeActor::SetSkyScale(float Scale)
{
	SkyScale = Scale;
	SetActorScale(FVector(Scale));
}

void ASkyDomeActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

	for (UActorComponent* OwnedComponent : OwnedComponents)
	{
		if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(OwnedComponent))
		{
			SkyMeshComponent = StaticMeshComponent;
			break;
		}
	}
}

void ASkyDomeActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadString(InOutHandle, "CurrentTexturePath", CurrentTexturePath, CurrentTexturePath, false);
		FJsonSerializer::ReadString(InOutHandle, "SkyMeshPath", SkyMeshPath, SkyMeshPath, false);
		FJsonSerializer::ReadFloat(InOutHandle, "SkyScale", SkyScale, SkyScale, false);
	}
	else
	{
		InOutHandle["CurrentTexturePath"] = CurrentTexturePath;
		InOutHandle["SkyMeshPath"] = SkyMeshPath;
		InOutHandle["SkyScale"] = SkyScale;
	}
}

void ASkyDomeActor::OnSerialized()
{
	Super::OnSerialized();

	SkyMeshComponent = Cast<UStaticMeshComponent>(RootComponent);
}

void ASkyDomeActor::InitializeSkyDome()
{
	if (SkyMeshComponent)
	{
		SkyMeshComponent->SetStaticMesh(SkyMeshPath);
		SkyMeshComponent->SetMaterialByName(0, "Shaders/Materials/SkyDome.hlsl");
		SetActorScale(FVector(SkyScale));
		SetSkyTexture(CurrentTexturePath);
	}
}