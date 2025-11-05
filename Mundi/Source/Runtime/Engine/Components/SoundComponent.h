#pragma once
#include "ActorComponent.h"
#include <xaudio2.h>

// Forward declarations
class USound;

/**
 * USoundComponent
 * Plays sound using XAudio2 directly
 * Each component owns its own SourceVoice for independent playback
 * Supports auto-play, looping, and volume control
 */
class USoundComponent : public UActorComponent
{
public:
	DECLARE_CLASS(USoundComponent, UActorComponent)
	GENERATED_REFLECTION_BODY()

	USoundComponent();

protected:
	~USoundComponent() override;

public:
	//================================================
	// Life Cycle
	//================================================

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason Reason) override;

	//================================================
	// Sound Control API
	//================================================

	/**
	 * Play the sound specified in SoundFilePath
	 */
	void Play();

	/**
	 * Stop the currently playing sound
	 */
	void Stop();

	/**
	 * Pause the currently playing sound
	 */
	void Pause();

	/**
	 * Resume the paused sound
	 */
	void Resume();

	//================================================
	// Settings
	//================================================

	void SetSound(USound* InSound) { Sound = InSound; }
	USound* GetSound() const { return Sound; }

	void SetAutoPlay(bool bInAutoPlay) { bAutoPlay = bInAutoPlay; }
	bool GetAutoPlay() const { return bAutoPlay; }

	void SetLoop(bool bInLoop) { bLoop = bInLoop; }
	bool GetLoop() const { return bLoop; }

	void SetVolume(float InVolume);
	float GetVolume() const { return Volume; }

	//================================================
	// Status
	//================================================

	bool IsPlaying() const;

	// Serialization API
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	void OnSerialized() override;

	// ───── 복사 관련 ────────────────────────────
	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(USoundComponent)
protected:
	//================================================
	// Properties (exposed to editor)
	//================================================

	/** Sound resource to play */
	USound* Sound;

	/** Whether to automatically play on BeginPlay */
	bool bAutoPlay;

	/** Whether to loop the sound */
	bool bLoop;

	/** Volume level (0.0 to 1.0) */
	float Volume;

private:
	/** XAudio2 source voice for playback (owned by this component) */
	IXAudio2SourceVoice* SourceVoice;
};
