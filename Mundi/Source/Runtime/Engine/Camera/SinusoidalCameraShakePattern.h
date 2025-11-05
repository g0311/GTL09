#pragma once
#include "CameraShakePattern.h"

// Simple sinusoidal camera shake pattern (location, rotation, FOV)
class SinusoidalCameraShakePattern : public UCameraShakePattern
{
public:
    DECLARE_CLASS(SinusoidalCameraShakePattern, UCameraShakePattern)
    GENERATED_REFLECTION_BODY()

    SinusoidalCameraShakePattern();
    virtual ~SinusoidalCameraShakePattern() override = default;

protected:
    void OnStart() override;
    void OnUpdate(const FCameraShakeUpdateParams& Params, FCameraShakeUpdateResult& OutResult) override;

public:
    // Location parameters
    FVector LocationAmplitude{0,0,0};     // units
    FVector LocationFrequency{0,0,0};     // Hz per axis
    FVector LocationPhase{0,0,0};         // radians per axis

    // Rotation parameters (degrees)
    FVector RotationAmplitudeDeg{0,0,0};  // degrees
    FVector RotationFrequency{0,0,0};     // Hz per axis
    FVector RotationPhase{0,0,0};         // radians per axis

    // FOV parameters (degrees)
    float FOVAmplitude = 0.0f;            // degrees
    float FOVFrequency = 0.0f;            // Hz
    float FOVPhase = 0.0f;                // radians

    // Utilities
    bool bRandomizePhase = false;         // if true, randomize phases at start
};

