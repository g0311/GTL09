#pragma once
#include "Object.h"
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

// 전방 선언
class USound;

/**
 * 재생 중인 사운드 인스턴스
 */
struct FSoundInstance
{
	IXAudio2SourceVoice* SourceVoice;
	bool bIsLooping;

	FSoundInstance() : SourceVoice(nullptr), bIsLooping(false) {}
};

/**
 * XAudio2 기반 사운드 재생 관리자
 *
 * - 사운드 리소스는 ResourceManager가 관리 (USound)
 * - 이 클래스는 XAudio2를 통한 재생/제어만 담당
 * - 싱글톤 패턴
 */
class USoundManager : public UObject
{
public:
	DECLARE_CLASS(USoundManager, UObject)

	//================================================
	// 싱글톤
	//================================================
	static USoundManager& GetInstance();

	//================================================
	// 생명 주기
	//================================================
	bool Initialize();
	void Shutdown();

	//================================================
	// 리소스 로드 (ResourceManager 사용)
	//================================================

	/** 디렉토리의 모든 WAV 파일을 ResourceManager에 프리로드 */
	void Preload();

	//================================================
	// 사운드 재생
	//================================================

	/**
	 * 사운드 재생 (ResourceManager에서 자동 로드)
	 * @param FilePath 사운드 파일 경로
	 * @param bLoop 루프 재생 여부
	 * @param Volume 볼륨 (0.0 ~ 1.0)
	 */
	bool PlaySound(const FString& FilePath, bool bLoop = false, float Volume = 1.0f);

	//================================================
	// 사운드 제어
	//================================================

	void StopSound(const FString& FilePath);
	void StopAllSounds();
	void PauseSound(const FString& FilePath);
	void ResumeSound(const FString& FilePath);

	//================================================
	// 볼륨 제어
	//================================================

	void SetMasterVolume(float Volume);
	void SetSoundVolume(const FString& FilePath, float Volume);
	float GetMasterVolume() const { return MasterVolume; }

	//================================================
	// 상태 확인
	//================================================

	bool IsSoundPlaying(const FString& FilePath) const;

	//================================================
	// 생성자/소멸자
	//================================================

	USoundManager();
	virtual ~USoundManager() override;

	USoundManager(const USoundManager&) = delete;
	USoundManager& operator=(const USoundManager&) = delete;

private:
	//================================================
	// 내부 헬퍼
	//================================================

	IXAudio2SourceVoice* CreateSourceVoice(USound* Sound);
	void CleanupFinishedSounds();

	//================================================
	// XAudio2 객체
	//================================================

	IXAudio2* XAudio2Engine;
	IXAudio2MasteringVoice* MasteringVoice;

	//================================================
	// 재생 관리
	//================================================

	TMap<FString, FSoundInstance> PlayingSounds;
	float MasterVolume;
	bool bInitialized;
};
