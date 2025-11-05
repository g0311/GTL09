#pragma once
#include "ResourceBase.h"
#include <xaudio2.h>

// WAV 파일 사운드 리소스
class USound : public UResourceBase
{
public:
	DECLARE_CLASS(USound, UResourceBase)

	USound();
	virtual ~USound() override;

	// ResourceBase 인터페이스
	void Load(const FString& InFilePath, ID3D11Device* InDevice);

	// 사운드 데이터 접근
	const WAVEFORMATEXTENSIBLE& GetWaveFormat() const { return WaveFormat; }
	const XAUDIO2_BUFFER& GetBuffer() const { return Buffer; }
	const TArray<BYTE>& GetAudioData() const { return AudioData; }

private:
	WAVEFORMATEXTENSIBLE WaveFormat;
	XAUDIO2_BUFFER Buffer;
	TArray<BYTE> AudioData;
};
