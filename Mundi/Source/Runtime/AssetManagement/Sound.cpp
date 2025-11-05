#include "pch.h"
#include "Sound.h"
#include <fstream>

IMPLEMENT_CLASS(USound)

USound::USound()
	: WaveFormat{}
	, Buffer{}
{
}

USound::~USound()
{
	AudioData.clear();
}

void USound::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
	std::ifstream File(InFilePath, std::ios::binary);
	if (!File.is_open())
	{
		UE_LOG("USound: Failed to open file: %s\n", InFilePath.c_str());
		return;
	}

	// RIFF 헤더 읽기
	char ChunkID[4];
	uint32_t ChunkSize;
	File.read(ChunkID, 4);
	if (strncmp(ChunkID, "RIFF", 4) != 0)
	{
		UE_LOG("USound: Invalid WAV file (no RIFF): %s\n", InFilePath.c_str());
		return;
	}

	File.read(reinterpret_cast<char*>(&ChunkSize), 4);
	File.read(ChunkID, 4);
	if (strncmp(ChunkID, "WAVE", 4) != 0)
	{
		UE_LOG("USound: Invalid WAV file (no WAVE): %s\n", InFilePath.c_str());
		return;
	}

	// fmt 청크 찾기
	bool bFoundFmt = false;
	while (File.read(ChunkID, 4))
	{
		File.read(reinterpret_cast<char*>(&ChunkSize), 4);

		if (strncmp(ChunkID, "fmt ", 4) == 0)
		{
			File.read(reinterpret_cast<char*>(&WaveFormat), ChunkSize);
			bFoundFmt = true;
		}
		else if (strncmp(ChunkID, "data", 4) == 0)
		{
			if (!bFoundFmt)
			{
				UE_LOG("USound: fmt chunk not found before data: %s\n", InFilePath.c_str());
				return;
			}

			// 오디오 데이터 읽기
			AudioData.resize(ChunkSize);
			File.read(reinterpret_cast<char*>(AudioData.data()), ChunkSize);

			// 버퍼 설정
			Buffer.AudioBytes = AudioData.size();
			Buffer.pAudioData = AudioData.data();
			Buffer.Flags = XAUDIO2_END_OF_STREAM;
			Buffer.LoopCount = 0;

			File.close();
			UE_LOG("USound: Loaded successfully: %s\n", InFilePath.c_str());
			return;
		}
		else
		{
			// 알 수 없는 청크는 건너뛰기
			File.seekg(ChunkSize, std::ios::cur);
		}
	}

	UE_LOG("USound: data chunk not found: %s\n", InFilePath.c_str());
}
