#include "pch.h"
#include "SoundComponent.h"
#include "Source/Runtime/Engine/Sound/USoundManager.h"
#include "Source/Runtime/AssetManagement/Sound.h"
#include "ObjectFactory.h"
#include "Math.h"

IMPLEMENT_CLASS(USoundComponent)

BEGIN_PROPERTIES(USoundComponent)
	MARK_AS_COMPONENT("Sound", "사운드를 재생하는 컴포넌트입니다")
	ADD_PROPERTY_SOUND(USound*, Sound, "사운드 컴포넌트", true, "재생할 사운드 리소스입니다")
	ADD_PROPERTY(bool, bAutoPlay, "사운드 컴포넌트", true, "BeginPlay에서 자동으로 재생합니다")
	ADD_PROPERTY(bool, bLoop, "사운드 컴포넌트", true, "사운드를 반복 재생합니다")
	ADD_PROPERTY(float, Volume, "사운드 컴포넌트", true, "볼륨 크기 (0.0 ~ 1.0)")
END_PROPERTIES()

USoundComponent::USoundComponent()
	: Sound(nullptr)
	, bAutoPlay(false)
	, bLoop(false)
	, Volume(1.0f)
	, bIsCurrentlyPlaying(false)
{
	bCanEverTick = false; // Sound component doesn't need to tick
}

USoundComponent::~USoundComponent()
{
}

void USoundComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-play if enabled and sound is set
	if (bAutoPlay && Sound)
	{
		Play();
	}
}

void USoundComponent::EndPlay(EEndPlayReason Reason)
{
	// Stop the sound when component is being destroyed
	if (bIsCurrentlyPlaying)
	{
		Stop();
	}

	Super::EndPlay(Reason);
}

void USoundComponent::Play()
{
	if (!Sound)
	{
		UE_LOG("USoundComponent::Play: Sound is nullptr!\n");
		return;
	}

	const FString& SoundFilePath = Sound->GetFilePath();
	if (SoundFilePath.empty())
	{
		UE_LOG("USoundComponent::Play: Sound file path is empty!\n");
		return;
	}

	USoundManager& SoundManager = USoundManager::GetInstance();

	if (SoundManager.PlaySound(SoundFilePath, bLoop, Volume))
	{
		bIsCurrentlyPlaying = true;
	}
	else
	{
		UE_LOG("USoundComponent::Play: Failed to play sound: %s\n", SoundFilePath.c_str());
	}
}

void USoundComponent::Stop()
{
	if (!bIsCurrentlyPlaying || !Sound)
	{
		return;
	}

	USoundManager& SoundManager = USoundManager::GetInstance();
	SoundManager.StopSound(Sound->GetFilePath());
	bIsCurrentlyPlaying = false;
}

void USoundComponent::Pause()
{
	if (!bIsCurrentlyPlaying || !Sound)
	{
		return;
	}

	USoundManager& SoundManager = USoundManager::GetInstance();
	SoundManager.PauseSound(Sound->GetFilePath());
}

void USoundComponent::Resume()
{
	if (!bIsCurrentlyPlaying || !Sound)
	{
		return;
	}

	USoundManager& SoundManager = USoundManager::GetInstance();
	SoundManager.ResumeSound(Sound->GetFilePath());
}

void USoundComponent::SetVolume(float InVolume)
{
	Volume = FMath::Clamp(InVolume, 0.0f, 1.0f);

	// Update volume if currently playing
	if (bIsCurrentlyPlaying && Sound)
	{
		USoundManager& SoundManager = USoundManager::GetInstance();
		SoundManager.SetSoundVolume(Sound->GetFilePath(), Volume);
	}
}

bool USoundComponent::IsPlaying() const
{
	if (!bIsCurrentlyPlaying || !Sound)
	{
		return false;
	}

	USoundManager& SoundManager = USoundManager::GetInstance();
	return SoundManager.IsSoundPlaying(Sound->GetFilePath());
}
