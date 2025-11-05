#include "pch.h"
#include "SoundComponent.h"
#include "Source/Runtime/Engine/Sound/USoundManager.h"
#include "Source/Runtime/AssetManagement/Sound.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "JsonSerializer.h"
#include "ObjectFactory.h"
#include "Math.h"

IMPLEMENT_CLASS(USoundComponent)

BEGIN_PROPERTIES(USoundComponent)
	MARK_AS_COMPONENT("사운드 컴포넌트", "사운드를 재생하는 컴포넌트입니다")
	ADD_PROPERTY_SOUND(USound*, Sound, "사운드 컴포넌트", true, "재생할 사운드 리소스입니다")
	ADD_PROPERTY(bool, bAutoPlay, "사운드 컴포넌트", true, "BeginPlay에서 자동으로 재생합니다")
	ADD_PROPERTY(bool, bLoop, "사운드 컴포넌트", true, "사운드를 반복 재생합니다")
	ADD_PROPERTY_RANGE(float, Volume, "사운드 컴포넌트", 0.f, 1.f, true, "볼륨 크기 (0.0 ~ 1.0)")
END_PROPERTIES()

USoundComponent::USoundComponent()
	: Sound(nullptr)
	, bAutoPlay(false)
	, bLoop(false)
	, Volume(1.0f)
	, SourceVoice(nullptr)
{
	bCanEverTick = false; // Sound component doesn't need to tick
}

USoundComponent::~USoundComponent()
{
	// Clean up source voice
	if (SourceVoice)
	{
		SourceVoice->Stop(0);
		SourceVoice->DestroyVoice();
		SourceVoice = nullptr;
	}
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
	// Stop and cleanup source voice
	Stop();

	Super::EndPlay(Reason);
}

void USoundComponent::Play()
{
	if (!Sound)
	{
		UE_LOG("USoundComponent::Play: Sound is nullptr!\n");
		return;
	}

	// Stop existing playback first
	Stop();

	USoundManager& SoundManager = USoundManager::GetInstance();

	// Create source voice
	SourceVoice = SoundManager.CreateSourceVoice(Sound);
	if (!SourceVoice)
	{
		UE_LOG("USoundComponent::Play: Failed to create source voice!\n");
		return;
	}

	// Setup buffer
	XAUDIO2_BUFFER Buffer = Sound->GetBuffer();
	if (bLoop)
	{
		Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	else
	{
		Buffer.LoopCount = 0;
	}

	// Submit buffer
	HRESULT hr = SourceVoice->SubmitSourceBuffer(&Buffer);
	if (FAILED(hr))
	{
		SourceVoice->DestroyVoice();
		SourceVoice = nullptr;
		UE_LOG("USoundComponent::Play: Failed to submit source buffer!\n");
		return;
	}

	// Set volume
	SourceVoice->SetVolume(Volume * SoundManager.GetMasterVolume());

	// Start playback
	hr = SourceVoice->Start(0);
	if (FAILED(hr))
	{
		SourceVoice->DestroyVoice();
		SourceVoice = nullptr;
		UE_LOG("USoundComponent::Play: Failed to start playback!\n");
		return;
	}
}

void USoundComponent::Stop()
{
	if (SourceVoice)
	{
		SourceVoice->Stop(0);
		SourceVoice->DestroyVoice();
		SourceVoice = nullptr;
	}
}

void USoundComponent::Pause()
{
	if (SourceVoice)
	{
		SourceVoice->Stop(0);
	}
}

void USoundComponent::Resume()
{
	if (SourceVoice)
	{
		SourceVoice->Start(0);
	}
}

void USoundComponent::SetVolume(float InVolume)
{
	Volume = FMath::Clamp(InVolume, 0.0f, 1.0f);

	// Update volume if currently playing
	if (SourceVoice)
	{
		USoundManager& SoundManager = USoundManager::GetInstance();
		SourceVoice->SetVolume(Volume * SoundManager.GetMasterVolume());
	}
}

bool USoundComponent::IsPlaying() const
{
	if (!SourceVoice)
	{
		return false;
	}

	XAUDIO2_VOICE_STATE State;
	SourceVoice->GetState(&State);
	return State.BuffersQueued > 0;
}

void USoundComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading) // 로드
	{
		FString SoundPath;
		FJsonSerializer::ReadString(InOutHandle, "SoundAssetPath", SoundPath, "", false);

		if (!SoundPath.empty())
		{
			Sound = UResourceManager::GetInstance().Load<USound>(SoundPath);
		}
		else
		{
			Sound = nullptr;
		}
	}
	else // 저장
	{
		if (Sound)
		{
			InOutHandle["SoundAssetPath"] = Sound->GetFilePath();
		}
		else
		{
			InOutHandle["SoundAssetPath"] = "";
		}
	}
}

void USoundComponent::OnSerialized()
{
	Super::OnSerialized();
}

void USoundComponent::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
}
