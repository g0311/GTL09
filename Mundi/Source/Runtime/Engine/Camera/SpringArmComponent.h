#pragma once
#include "SceneComponent.h"

class USpringArmComponent : public USceneComponent
{
public:
    DECLARE_CLASS(USpringArmComponent, USceneComponent)
    GENERATED_REFLECTION_BODY()

    USpringArmComponent();
    ~USpringArmComponent() override = default;

    // Length from arm origin to camera (along -X in local space)
    float TargetArmLength;

    // Local-space offset applied at the end of the arm
    FVector TargetOffset;

    // Reserved for future collision sweeps (not used yet)
    float ProbeSize;

    // Control/Inheritance (no APawn dependency; flags only)
    bool bUsePawnControlRotation = false;
    bool bInheritPitch = true;
    bool bInheritYaw = true;
    bool bInheritRoll = true;

    // Lag options
    bool bEnableCameraLag = true;
    bool bEnableCameraRotationLag = true;
    float CameraLagSpeed;
    float CameraRotationLagSpeed;
    float CameraLagMaxDistance; // 0 = unlimited

    // Query API
    FVector GetDesiredRotation() const;  // current component rotation (Euler ZYX)
    FVector GetTargetRotation() const;   // last computed camera rotation (Euler ZYX)
    FVector GetUnfixedCameraPosition() const; // camera pos before collision fix

    bool IsCollisionFixApplied() const;

    // Lifecycle
    void OnRegister(UWorld* InWorld) override;
    void TickComponent(float DeltaTime) override;

    // Copy/Serialize
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(USpringArmComponent)
    void OnSerialized() override;

protected:
    void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime);
    FVector BlendLocations(const FVector& From, const FVector& To, float DeltaTime, float LagSpeed, float MaxDistance);

private:
    // Cached results for queries and lag
    bool bIsCameraFixed = false;
    bool bFirstTick = true;
    FVector UnfixedCameraPosition{0, 0, 0};

    FVector PreviousDesiredLocation{0, 0, 0};
    FVector PreviousArmOrigin{0, 0, 0};
    FQuat   PreviousDesiredRotation{0, 0, 0, 1};
    FQuat   SmoothedRotation{0, 0, 0, 1};
};
