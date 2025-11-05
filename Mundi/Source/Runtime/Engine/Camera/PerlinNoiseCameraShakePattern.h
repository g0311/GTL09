#pragma once
#include "CameraShakePattern.h"

// Fractal Perlin-like 1D noise camera shake (per-axis)
class UPerlinNoiseCameraShakePattern : public UCameraShakePattern
{
public:
    DECLARE_CLASS(UPerlinNoiseCameraShakePattern, UCameraShakePattern)
    GENERATED_REFLECTION_BODY()

    UPerlinNoiseCameraShakePattern() = default;
    virtual ~UPerlinNoiseCameraShakePattern() override = default;

protected:
    void OnStart() override {}
    void OnUpdate(const FCameraShakeUpdateParams& Params, FCameraShakeUpdateResult& OutResult) override;

private:
    // 1D Perlin-style noise helpers
    static inline float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    static inline uint32 HashInt(int32 x, int32 seed)
    {
        uint32 h = static_cast<uint32>(x);
        h ^= static_cast<uint32>(seed) + 0x9e3779b9u + (h << 6) + (h >> 2);
        h = (h ^ 61u) ^ (h >> 16);
        h *= 9u;
        h = h ^ (h >> 4);
        h *= 0x27d4eb2du;
        h = h ^ (h >> 15);
        return h;
    }
    static inline float Grad1D(uint32 h, float x)
    {
        // Two gradients: +1 or -1
        return (h & 1u) ? x : -x;
    }
    static inline float Perlin1D(float x, int32 seed)
    {
        int32 xi = static_cast<int32>(std::floor(x));
        float xf = x - static_cast<float>(xi);
        float u = Fade(xf);
        float g0 = Grad1D(HashInt(xi, seed), xf);
        float g1 = Grad1D(HashInt(xi + 1, seed), xf - 1.0f);
        return Lerp(g0, g1, u); // roughly [-1,1]
    }
    static inline float Fractal1D(float x, int32 seed, int32 octaves, float lacunarity, float persistence)
    {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;
        for (int i = 0; i < octaves; ++i)
        {
            sum += Perlin1D(x * freq, seed + i * 101) * amp;
            norm += amp;
            amp *= persistence;
            freq *= lacunarity;
        }
        if (norm > 0.0f) sum /= norm;
        return sum; // [-1,1]
    }

public:
    // Location parameters
    FVector LocationAmplitude{ 0,0,0 };  // units
    FVector LocationFrequency{ 0,0,0 };  // Hz per axis (base frequency)

    // Rotation parameters (degrees)
    FVector RotationAmplitudeDeg{ 0,0,0 }; // degrees
    FVector RotationFrequency{ 0,0,0 };    // Hz per axis (base frequency)

    // FOV parameters (degrees)
    float FOVAmplitude = 0.0f;             // degrees
    float FOVFrequency = 0.0f;             // Hz

    // Fractal controls
    int32  Octaves = 3;
    float  Lacunarity = 2.0f;   // freq multiplier per octave
    float  Persistence = 0.5f;  // amp multiplier per octave
    int32  Seed = 1337;         // base seed
};