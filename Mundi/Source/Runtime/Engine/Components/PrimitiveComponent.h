#pragma once
#include "SceneComponent.h"
#include "Material.h"
#include "Delegate.h"

// 전방 선언
struct FSceneCompData;

class URenderer;
struct FMeshBatchElement;
class FSceneView;

struct FOverlapInfo
{
    AActor* OtherActor = nullptr;
    UPrimitiveComponent* OtherComp = nullptr;

    bool operator==(const FOverlapInfo& Other) const
    {
        return OtherActor == Other.OtherActor && OtherComp == Other.OtherComp;
    }
};

// 충돌 지점 정보 (Billboard 이펙트 등에 사용)
struct FContactInfo
{
    FVector ContactPoint = FVector::Zero();  // 충돌 위치
    FVector ContactNormal = FVector::Zero(); // 충돌 방향 (첫 번째 물체 기준)
    float PenetrationDepth = 0.0f;               // 관통 깊이 (optional)

    FContactInfo() = default;
    FContactInfo(const FVector& Point, const FVector& Normal, float Depth = 0.0f)
        : ContactPoint(Point), ContactNormal(Normal), PenetrationDepth(Depth) {}
};

class UPrimitiveComponent :public USceneComponent
{
public:
    DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

    UPrimitiveComponent() = default;
    virtual ~UPrimitiveComponent() = default;

    // 이 프리미티브를 렌더링하는 데 필요한 FMeshBatchElement를 수집합니다.
    virtual void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) {}

    virtual UMaterialInterface* GetMaterial(uint32 InElementIndex) const
    {
        // 기본 구현: UPrimitiveComponent 자체는 머티리얼을 소유하지 않으므로 nullptr 반환
        return nullptr;
    }
    virtual void SetMaterial(uint32 InElementIndex, UMaterialInterface* InNewMaterial)
    {
        // 기본 구현: 아무것도 하지 않음 (머티리얼을 지원하지 않거나 설정 불가)
    }

    // 내부적으로 ResourceManager를 통해 UMaterial*를 찾아 SetMaterial을 호출합니다.
    void SetMaterialByName(uint32 InElementIndex, const FString& MaterialName);


    void SetCulled(bool InCulled)
    {
        bIsCulled = InCulled;
    }

    bool GetCulled() const
    {
        return bIsCulled;
    }

    // ───── 복사 관련 ────────────────────────────
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UPrimitiveComponent)

    // ───── 직렬화 ────────────────────────────
    virtual void OnSerialized() override;

    
    // ────────────────
    // Collision System
    // ────────────────

    // setters
    void SetCollisionEnabled(bool bInCollisionEnabled) { bIsCollisionEnabled = bInCollisionEnabled; }
    void SetGenerateOverlapEvents(bool bInGenerateOverlapEvents) { bGenerateOverlapEvents = bInGenerateOverlapEvents; }
    void SetBlockComponent(bool bInBlockComponent) { bBlockComponent = bInBlockComponent; }
    void SetCollisionLayer(uint32 InCollisionLayer) { CollisionLayer = InCollisionLayer; }
    
    // getters
    bool IsCollisionEnabled() const { return bIsCollisionEnabled; }
    bool GetGenerateOverlapEvents() const { return bGenerateOverlapEvents; }
    bool GetBlockComponent() const { return bBlockComponent; }
    uint32 GetCollisionLayer() const { return CollisionLayer; }
    const TArray<FOverlapInfo>& GetOverlapInfos() const { return OverlapInfos; };

    bool IsOverlappingActor(const AActor* Other) const;
    TArray<AActor*> GetOverlappingActors(TArray<AActor*>& OutActors, uint32 mask=~0u) const;
    void RefreshOverlapInfos(uint32 mask = ~0u);

    // Overlap events (begin/end). Users can register handlers.
    DECLARE_DELEGATE(FOnOverlapSignature, UPrimitiveComponent*, AActor*, UPrimitiveComponent*, const FContactInfo&);

    /**
     * Register a handler to be invoked when this component begins overlapping another.
     *
     * @param Handler Delegate callback to register; it will be called with (this component, other actor, other component, contact info) when a begin-overlap occurs.
     * @returns Handle that can be used to remove the registered delegate.
     */
    FDelegateHandle AddOnBeginOverlap(const FOnOverlapSignature::HandlerType& Handler)
    {
        return OnBeginOverlapDelegate.Add(Handler);
    }

    /**
     * Register a handler to be invoked when this component ends an overlap with another.
     * @param Handler Delegate handler to call when an end-overlap event occurs.
     * @returns FDelegateHandle Handle that can be used to unregister the handler.
     */
    FDelegateHandle AddOnEndOverlap(const FOnOverlapSignature::HandlerType& Handler)
    {
        return OnEndOverlapDelegate.Add(Handler);
    }

    template<typename T>
    FDelegateHandle AddOnBeginOverlapDynamic(T* Instance, void (T::*Func)(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, const FContactInfo&))
    {
        return OnBeginOverlapDelegate.AddDynamic(Instance, Func);
    }

    template<typename T>
    FDelegateHandle AddOnEndOverlapDynamic(T* Instance, void (T::*Func)(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, const FContactInfo&))
    {
        return OnEndOverlapDelegate.AddDynamic(Instance, Func);
    }

    // Lua 바인딩용: Listener 포인터를 지정하여 등록
    FDelegateHandle AddOnBeginOverlapWithListener(const FOnOverlapSignature::HandlerType& Handler, void* ListenerPtr)
    {
        return OnBeginOverlapDelegate.AddWithListener(Handler, ListenerPtr);
    }

    FDelegateHandle AddOnEndOverlapWithListener(const FOnOverlapSignature::HandlerType& Handler, void* ListenerPtr)
    {
        return OnEndOverlapDelegate.AddWithListener(Handler, ListenerPtr);
    }

    bool RemoveOnBeginOverlap(FDelegateHandle Handle) { return OnBeginOverlapDelegate.Remove(Handle); }
    /**
 * Unregisters a previously added end-overlap delegate.
 * @param Handle Delegate handle identifying the registration to remove.
 * @returns `true` if a matching delegate was found and removed, `false` otherwise.
 */
bool RemoveOnEndOverlap(FDelegateHandle Handle) { return OnEndOverlapDelegate.Remove(Handle); }

    /**
     * Notify all registered begin-overlap handlers about a new overlap and provide contact details.
     *
     * @param OtherActor Actor that began overlapping with this component.
     * @param OtherComp  Component on the other actor involved in the overlap.
     * @param ContactInfo Struct containing contact point, contact normal (from this component), and optional penetration depth.
     */
    void BroadcastBeginOverlap(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FContactInfo& ContactInfo)
    {
        OnBeginOverlapDelegate.Broadcast(this, OtherActor, OtherComp, ContactInfo);
    }

    /**
     * Broadcasts an end-overlap event to all registered end-overlap handlers.
     *
     * @param OtherActor The other actor involved in the overlap that ended (may be nullptr).
     * @param OtherComp The other component involved in the overlap that ended (may be nullptr).
     * @param ContactInfo Contact information describing the overlap event (contact point, normal, and penetration).
     */
    void BroadcastEndOverlap(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FContactInfo& ContactInfo)
    {
        OnEndOverlapDelegate.Broadcast(this, OtherActor, OtherComp, ContactInfo);
    }

    FOnOverlapSignature OnBeginOverlapDelegate;
    FOnOverlapSignature OnEndOverlapDelegate;

protected:
    bool bIsCulled = false;

    bool bIsCollisionEnabled = true;
    bool bGenerateOverlapEvents = true;
    bool bBlockComponent = false;

    uint32 CollisionLayer = 0u;
    TArray<FOverlapInfo> OverlapInfos;
};