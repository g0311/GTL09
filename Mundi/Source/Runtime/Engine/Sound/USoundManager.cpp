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

	// COM 초기화 (main에서 이미 초기화되었을 수 있음)
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr) && hr != RPC_E_CHANGED_MODE && hr != S_FALSE)
	{
		// RPC_E_CHANGED_MODE: 다른 스레딩 모델로 이미 초기화됨 (허용)
		// S_FALSE: 이미 초기화됨 (허용)
		UE_LOG("SoundManager: CoInitialize failed with unexpected error!\n");
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

// 마스터 볼륨 설정
void USoundManager::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (MasteringVoice)
	{
		MasteringVoice->SetVolume(MasterVolume);
	}
}
