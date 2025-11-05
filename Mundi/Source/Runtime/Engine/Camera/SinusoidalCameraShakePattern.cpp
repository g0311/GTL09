#include "pch.h"
#include "SinusoidalCameraShakePattern.h"
#include <cstdlib>

IMPLEMENT_CLASS(SinusoidalCameraShakePattern)

BEGIN_PROPERTIES(SinusoidalCameraShakePattern)
    // Location
    ADD_PROPERTY(FVector, LocationAmplitude, "CameraShake|Sin", true, "Location amplitude (units)")
    ADD_PROPERTY(FVector, LocationFrequency, "CameraShake|Sin", true, "Location frequency (Hz)")
    ADD_PROPERTY(FVector, LocationPhase,     "CameraShake|Sin", true, "Location phase (radians)")
    // Rotation
    ADD_PROPERTY(FVector, RotationAmplitudeDeg, "CameraShake|Sin", true, "Rotation amplitude (degrees)")
    ADD_PROPERTY(FVector, RotationFrequency,    "CameraShake|Sin", true, "Rotation frequency (Hz)")
    ADD_PROPERTY(FVector, RotationPhase,        "CameraShake|Sin", true, "Rotation phase (radians)")
    // FOV
    ADD_PROPERTY_RANGE(float, FOVAmplitude, "CameraShake|Sin", 0.0f, 180.0f, true, "FOV amplitude (degrees)")
    ADD_PROPERTY_RANGE(float, FOVFrequency, "CameraShake|Sin", 0.0f, 100.0f, true, "FOV frequency (Hz)")
    ADD_PROPERTY_RANGE(float, FOVPhase,     "CameraShake|Sin", -1000.0f, 1000.0f, true, "FOV phase (radians)")
    // Misc
    ADD_PROPERTY(bool, bRandomizePhase, "CameraShake|Sin", true, "Randomize phases at Start")
END_PROPERTIES()

SinusoidalCameraShakePattern::SinusoidalCameraShakePattern()
{
}

static inline float TwoPi() { return 6.28318530717958647692f; }

void SinusoidalCameraShakePattern::OnStart()
{
    if (bRandomizePhase)
    {
        auto rnd = []() -> float { return ((float)std::rand() / (float)RAND_MAX) * TwoPi(); };
        LocationPhase = FVector(rnd(), rnd(), rnd());
        RotationPhase = FVector(rnd(), rnd(), rnd());
        FOVPhase = rnd();
    }
}

void SinusoidalCameraShakePattern::OnUpdate(const FCameraShakeUpdateParams& Params, FCameraShakeUpdateResult& OutResult)
{
    const float t = Params.ElapsedTime;

    // Location offset
    auto axis = [&](float amp, float freq, float phase) -> float
    {
        const float w = TwoPi() * freq;
        return amp * std::sin(w * t + phase);
    };

    OutResult.LocationOffset.X = axis(LocationAmplitude.X, std::max(0.0f, LocationFrequency.X), LocationPhase.X);
    OutResult.LocationOffset.Y = axis(LocationAmplitude.Y, std::max(0.0f, LocationFrequency.Y), LocationPhase.Y);
    OutResult.LocationOffset.Z = axis(LocationAmplitude.Z, std::max(0.0f, LocationFrequency.Z), LocationPhase.Z);

    // Rotation offset: produce small Euler angles (degrees)
    FVector eulerDeg;
    eulerDeg.X = axis(RotationAmplitudeDeg.X, std::max(0.0f, RotationFrequency.X), RotationPhase.X);
    eulerDeg.Y = axis(RotationAmplitudeDeg.Y, std::max(0.0f, RotationFrequency.Y), RotationPhase.Y);
    eulerDeg.Z = axis(RotationAmplitudeDeg.Z, std::max(0.0f, RotationFrequency.Z), RotationPhase.Z);

    OutResult.RotationOffset = FQuat::MakeFromEulerZYX(eulerDeg).GetNormalized();

    // FOV offset (degrees)
    OutResult.FOVOffset = axis(FOVAmplitude, std::max(0.0f, FOVFrequency), FOVPhase);
}
