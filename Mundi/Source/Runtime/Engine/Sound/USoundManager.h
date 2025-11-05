#pragma once
#include "Object.h"
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

// 전방 선언
class USound;

/**
 * XAudio2 기반 사운드 엔진 관리자
 *
 * - 사운드 리소스는 ResourceManager가 관리 (USound)
 * - 이 클래스는 XAudio2 엔진 초기화 및 SourceVoice 생성만 담당
 * - 실제 재생은 SoundComponent가 SourceVoice를 소유하여 직접 제어
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
	// SourceVoice 생성 (Components가 사용)
	//================================================

	/**
	 * Create a source voice for a sound
	 * @param Sound Sound resource to create voice for
	 * @return Created source voice (caller owns and must destroy)
	 */
	IXAudio2SourceVoice* CreateSourceVoice(USound* Sound);

	//================================================
	// 볼륨 제어
	//================================================

	void SetMasterVolume(float Volume);
	float GetMasterVolume() const { return MasterVolume; }

	//================================================
	// 생성자/소멸자
	//================================================

	USoundManager();
	virtual ~USoundManager() override;

	USoundManager(const USoundManager&) = delete;
	USoundManager& operator=(const USoundManager&) = delete;

private:
	//================================================
	// XAudio2 객체
	//================================================

	IXAudio2* XAudio2Engine;
	IXAudio2MasteringVoice* MasteringVoice;

	//================================================
	// 상태
	//================================================

	float MasterVolume;
	bool bInitialized;
};
