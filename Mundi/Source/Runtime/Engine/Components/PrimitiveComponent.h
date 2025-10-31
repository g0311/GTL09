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
    DECLARE_DELEGATE(FOnOverlapSignature, UPrimitiveComponent*, AActor*, UPrimitiveComponent*);

    FDelegateHandle AddOnBeginOverlap(const FOnOverlapSignature::HandlerType& Handler)
    {
        return OnBeginOverlapDelegate.Add(Handler);
    }

    FDelegateHandle AddOnEndOverlap(const FOnOverlapSignature::HandlerType& Handler)
    {
        return OnEndOverlapDelegate.Add(Handler);
    }

    template<typename T>
    FDelegateHandle AddOnBeginOverlapDynamic(T* Instance, void (T::*Func)(UPrimitiveComponent*, AActor*, UPrimitiveComponent*))
    {
        return OnBeginOverlapDelegate.AddDynamic(Instance, Func);
    }

    template<typename T>
    FDelegateHandle AddOnEndOverlapDynamic(T* Instance, void (T::*Func)(UPrimitiveComponent*, AActor*, UPrimitiveComponent*))
    {
        return OnEndOverlapDelegate.AddDynamic(Instance, Func);
    }

    bool RemoveOnBeginOverlap(FDelegateHandle Handle) { return OnBeginOverlapDelegate.Remove(Handle); }
    bool RemoveOnEndOverlap(FDelegateHandle Handle) { return OnEndOverlapDelegate.Remove(Handle); }

    // Broadcasting is intended for the collision manager
    void BroadcastBeginOverlap(AActor* OtherActor, UPrimitiveComponent* OtherComp)
    {
        OnBeginOverlapDelegate.Broadcast(this, OtherActor, OtherComp);
    }

    void BroadcastEndOverlap(AActor* OtherActor, UPrimitiveComponent* OtherComp)
    {
        OnEndOverlapDelegate.Broadcast(this, OtherActor, OtherComp);
    }

protected:
    bool bIsCulled = false;

    bool bIsCollisionEnabled = true;
    bool bGenerateOverlapEvents = true;
    bool bBlockComponent = false;

    uint32 CollisionLayer = 0u;
    TArray<FOverlapInfo> OverlapInfos;

    FOnOverlapSignature OnBeginOverlapDelegate;
    FOnOverlapSignature OnEndOverlapDelegate;
};
