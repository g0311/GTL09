#pragma once
#include "ActorComponent.h"

/**
 * USoundComponent
 * Plays sound using USoundManager
 * Supports auto-play, looping, and volume control
 */
class USoundComponent : public UActorComponent
{
public:
	DECLARE_CLASS(USoundComponent, UActorComponent)
	GENERATED_REFLECTION_BODY()
	DECLARE_DUPLICATE(USoundComponent)

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

	void SetSoundFilePath(const FString& InFilePath) { SoundFilePath = InFilePath; }
	const FString& GetSoundFilePath() const { return SoundFilePath; }

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

protected:
	//================================================
	// Properties (exposed to editor)
	//================================================

	/** Path to the sound file to play */
	FString SoundFilePath;

	/** Whether to automatically play on BeginPlay */
	bool bAutoPlay;

	/** Whether to loop the sound */
	bool bLoop;

	/** Volume level (0.0 to 1.0) */
	float Volume;

private:
	/** Track if we are currently managing a sound */
	bool bIsCurrentlyPlaying;
};
