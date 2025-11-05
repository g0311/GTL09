#include "pch.h"
#include "USoundManager.h"
#include "Sound.h"
#include "ResourceManager.h"
#include <filesystem>

// 싱글톤 인스턴스
USoundManager& USoundManager::GetInstance()
{
	static USoundManager Instance;
	return Instance;
}

// 생성자
USoundManager::USoundManager()
	: XAudio2Engine(nullptr)
	, MasteringVoice(nullptr)
	, MasterVolume(1.0f)
	, bInitialized(false)
{
}

// 소멸자
USoundManager::~USoundManager()
{
	Shutdown();
}

// 초기화
bool USoundManager::Initialize()
{
	if (bInitialized)
	{
		return true;
	}

	// COM 초기화
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		UE_LOG("SoundManager: CoInitialize failed!\n");
		return false;
	}

	// XAudio2 엔진 생성
	hr = XAudio2Create(&XAudio2Engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		UE_LOG("SoundManager: XAudio2Create failed!\n");
		CoUninitialize();
		return false;
	}

	// 마스터링 보이스 생성
	hr = XAudio2Engine->CreateMasteringVoice(&MasteringVoice);
	if (FAILED(hr))
	{
		UE_LOG("SoundManager: CreateMasteringVoice failed!\n");
		XAudio2Engine->Release();
		XAudio2Engine = nullptr;
		CoUninitialize();
		return false;
	}

	bInitialized = true;
	UE_LOG("SoundManager: Initialized successfully!\n");
	return true;
}

// 종료
void USoundManager::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

	StopAllSounds();

	if (MasteringVoice)
	{
		MasteringVoice->DestroyVoice();
		MasteringVoice = nullptr;
	}

	if (XAudio2Engine)
	{
		XAudio2Engine->Release();
		XAudio2Engine = nullptr;
	}

	CoUninitialize();
	bInitialized = false;
	UE_LOG("SoundManager: Shutdown complete!\n");
}

void USoundManager::Preload()
{
	if (!bInitialized)
	{
		UE_LOG("SoundManager: Not initialized!\n");
		return;
	}

	namespace fs = std::filesystem;

	const fs::path SoundDir("Sound");

	if (!fs::exists(SoundDir) || !fs::is_directory(SoundDir))
	{
		UE_LOG("USoundManager::Preload: Sound directory not found: %s\n", SoundDir.string().c_str());
		return;
	}

	size_t LoadedCount = 0;
	std::unordered_set<FString> ProcessedFiles; // 중복 로딩 방지

	for (const auto& Entry : fs::recursive_directory_iterator(SoundDir))
	{
		if (!Entry.is_regular_file())
			continue;

		const fs::path& Path = Entry.path();
		FString Extension = Path.extension().string();
		std::transform(Extension.begin(), Extension.end(), Extension.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (Extension == ".wav")
		{
			FString PathStr = NormalizePath(Path.string());

			// 이미 처리된 파일인지 확인
			if (ProcessedFiles.find(PathStr) == ProcessedFiles.end())
			{
				ProcessedFiles.insert(PathStr);

				// ResourceManager를 통해 프리로드
				USound* Sound = RESOURCE.Load<USound>(PathStr);
				if (Sound)
				{
					++LoadedCount;
				}
			}
		}
	}

	UE_LOG("USoundManager::Preload: Loaded %zu .wav files from %s\n",
		LoadedCount, SoundDir.string().c_str());
}

// 소스 보이스 생성
IXAudio2SourceVoice* USoundManager::CreateSourceVoice(USound* Sound)
{
	if (!XAudio2Engine || !Sound)
	{
		return nullptr;
	}

	IXAudio2SourceVoice* SourceVoice = nullptr;
	HRESULT hr = XAudio2Engine->CreateSourceVoice(&SourceVoice,
		reinterpret_cast<const WAVEFORMATEX*>(&Sound->GetWaveFormat()));

	if (FAILED(hr))
	{
		UE_LOG("SoundManager: Failed to create source voice!\n");
		return nullptr;
	}

	return SourceVoice;
}

// 사운드 재생
bool USoundManager::PlaySound(const FString& FilePath, bool bLoop, float Volume)
{
	if (!bInitialized)
	{
		return false;
	}

	// ResourceManager에서 사운드 로드 (없으면 자동 로드, 있으면 캐시 반환)
	USound* Sound = UResourceManager::GetInstance().Load<USound>(FilePath);
	if (!Sound)
	{
		UE_LOG("SoundManager: Failed to load sound: %s\n", FilePath.c_str());
		return false;
	}

	// 이미 재생 중이면 중지
	StopSound(FilePath);

	// 소스 보이스 생성
	IXAudio2SourceVoice* SourceVoice = CreateSourceVoice(Sound);
	if (!SourceVoice)
	{
		return false;
	}

	// 버퍼 설정
	XAUDIO2_BUFFER Buffer = Sound->GetBuffer();
	if (bLoop)
	{
		Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	else
	{
		Buffer.LoopCount = 0;
	}

	// 버퍼 제출 및 재생
	HRESULT hr = SourceVoice->SubmitSourceBuffer(&Buffer);
	if (FAILED(hr))
	{
		SourceVoice->DestroyVoice();
		UE_LOG("SoundManager: Failed to submit source buffer!\n");
		return false;
	}

	// 볼륨 설정
	SourceVoice->SetVolume(Volume * MasterVolume);

	// 재생 시작
	hr = SourceVoice->Start(0);
	if (FAILED(hr))
	{
		SourceVoice->DestroyVoice();
		UE_LOG("SoundManager: Failed to start playback!\n");
		return false;
	}

	// 재생 목록에 추가
	FSoundInstance Instance;
	Instance.SourceVoice = SourceVoice;
	Instance.bIsLooping = bLoop;
	PlayingSounds[FilePath] = Instance;

	return true;
}

// 사운드 정지
void USoundManager::StopSound(const FString& FilePath)
{
	auto Iter = PlayingSounds.find(FilePath);
	if (Iter != PlayingSounds.end())
	{
		if (Iter->second.SourceVoice)
		{
			Iter->second.SourceVoice->Stop(0);
			Iter->second.SourceVoice->DestroyVoice();
		}
		PlayingSounds.erase(Iter);
	}
}

// 모든 사운드 정지
void USoundManager::StopAllSounds()
{
	for (auto& Pair : PlayingSounds)
	{
		if (Pair.second.SourceVoice)
		{
			Pair.second.SourceVoice->Stop(0);
			Pair.second.SourceVoice->DestroyVoice();
		}
	}
	PlayingSounds.clear();
}

// 사운드 일시정지
void USoundManager::PauseSound(const FString& FilePath)
{
	auto Iter = PlayingSounds.find(FilePath);
	if (Iter != PlayingSounds.end() && Iter->second.SourceVoice)
	{
		Iter->second.SourceVoice->Stop(0);
	}
}

// 사운드 재개
void USoundManager::ResumeSound(const FString& FilePath)
{
	auto Iter = PlayingSounds.find(FilePath);
	if (Iter != PlayingSounds.end() && Iter->second.SourceVoice)
	{
		Iter->second.SourceVoice->Start(0);
	}
}

// 마스터 볼륨 설정
void USoundManager::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (MasteringVoice)
	{
		MasteringVoice->SetVolume(MasterVolume);
	}
}

// 개별 사운드 볼륨 설정
void USoundManager::SetSoundVolume(const FString& FilePath, float Volume)
{
	auto Iter = PlayingSounds.find(FilePath);
	if (Iter != PlayingSounds.end() && Iter->second.SourceVoice)
	{
		Iter->second.SourceVoice->SetVolume(FMath::Clamp(Volume, 0.0f, 1.0f) * MasterVolume);
	}
}

// 사운드 재생 중인지 확인
bool USoundManager::IsSoundPlaying(const FString& FilePath) const
{
	auto Iter = PlayingSounds.find(FilePath);
	if (Iter != PlayingSounds.end() && Iter->second.SourceVoice)
	{
		XAUDIO2_VOICE_STATE State;
		Iter->second.SourceVoice->GetState(&State);
		return State.BuffersQueued > 0;
	}
	return false;
}

// 완료된 사운드 정리
void USoundManager::CleanupFinishedSounds()
{
	TArray<FString> FinishedSounds;

	for (auto& Pair : PlayingSounds)
	{
		if (Pair.second.SourceVoice)
		{
			XAUDIO2_VOICE_STATE State;
			Pair.second.SourceVoice->GetState(&State);

			// 버퍼가 비어있고 루프가 아니면 완료된 것
			if (State.BuffersQueued == 0 && !Pair.second.bIsLooping)
			{
				FinishedSounds.push_back(Pair.first);
			}
		}
	}

	// 완료된 사운드 정리
	for (const auto& SoundPath : FinishedSounds)
	{
		StopSound(SoundPath);
	}
}
