#include "pch.h"
#include "PerlinNoiseCameraShakePattern.h"

IMPLEMENT_CLASS(UPerlinNoiseCameraShakePattern)

BEGIN_PROPERTIES(UPerlinNoiseCameraShakePattern)
    // Location
    ADD_PROPERTY(FVector, LocationAmplitude, "CameraShake|Perlin", true, "Location amplitude (units)")
    ADD_PROPERTY(FVector, LocationFrequency, "CameraShake|Perlin", true, "Location frequency (Hz)")
    // Rotation
    ADD_PROPERTY(FVector, RotationAmplitudeDeg, "CameraShake|Perlin", true, "Rotation amplitude (degrees)")
    ADD_PROPERTY(FVector, RotationFrequency,    "CameraShake|Perlin", true, "Rotation frequency (Hz)")
    // FOV
    ADD_PROPERTY_RANGE(float, FOVAmplitude, "CameraShake|Perlin", 0.0f, 180.0f, true, "FOV amplitude (degrees)")
    ADD_PROPERTY_RANGE(float, FOVFrequency, "CameraShake|Perlin", 0.0f, 100.0f, true, "FOV frequency (Hz)")
    // Fractal controls
    ADD_PROPERTY_RANGE(int32, Octaves,      "CameraShake|Perlin", 1, 8, true, "Number of octaves")
    ADD_PROPERTY_RANGE(float, Lacunarity,   "CameraShake|Perlin", 1.0f, 6.0f, true, "Frequency multiplier per octave")
    ADD_PROPERTY_RANGE(float, Persistence,  "CameraShake|Perlin", 0.0f, 1.0f, true, "Amplitude multiplier per octave")
    ADD_PROPERTY_RANGE(int32, Seed,         "CameraShake|Perlin", -1000000, 1000000, true, "Noise seed")
END_PROPERTIES()

void UPerlinNoiseCameraShakePattern::OnUpdate(const FCameraShakeUpdateParams& Params, FCameraShakeUpdateResult& OutResult)
{
    const float t = Params.ElapsedTime;

    auto noiseAxis = [&](float amp, float freq, int axis) -> float
    {
        if (amp == 0.0f || freq == 0.0f) return 0.0f;
        const int32 axisSeed = Seed + axis * 10007; // decorrelate per-axis
        const float n = Fractal1D(t * freq, axisSeed, Octaves, Lacunarity, Persistence);
        return amp * n; // [-amp, amp]
    };

    // Location
    OutResult.LocationOffset.X = noiseAxis(LocationAmplitude.X, std::max(0.0f, LocationFrequency.X), 0);
    OutResult.LocationOffset.Y = noiseAxis(LocationAmplitude.Y, std::max(0.0f, LocationFrequency.Y), 1);
    OutResult.LocationOffset.Z = noiseAxis(LocationAmplitude.Z, std::max(0.0f, LocationFrequency.Z), 2);

    // Rotation: build small Euler in degrees and convert
    FVector eulerDeg;
    eulerDeg.X = noiseAxis(RotationAmplitudeDeg.X, std::max(0.0f, RotationFrequency.X), 10);
    eulerDeg.Y = noiseAxis(RotationAmplitudeDeg.Y, std::max(0.0f, RotationFrequency.Y), 11);
    eulerDeg.Z = noiseAxis(RotationAmplitudeDeg.Z, std::max(0.0f, RotationFrequency.Z), 12);
    OutResult.RotationOffset = FQuat::MakeFromEulerZYX(eulerDeg).GetNormalized();

    // FOV
    OutResult.FOVOffset = noiseAxis(FOVAmplitude, std::max(0.0f, FOVFrequency), 20);
}