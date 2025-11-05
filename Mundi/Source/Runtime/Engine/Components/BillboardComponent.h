#pragma once
#include "PrimitiveComponent.h"
#include "Object.h"

class UQuad;
class UTexture;
class UMaterial;
class URenderer;

/**
 * Initialize a billboard component with default state.
 */
 
/**
 * Destroy the billboard component and release any owned runtime resources.
 */

/**
 * Collect rendering mesh batch elements for this billboard into OutMeshBatchElements for the given view.
 * @param OutMeshBatchElements Array populated with mesh batch elements used to render this component.
 * @param View Scene view driving LOD, visibility, and view-dependent batching decisions.
 */

/**
 * Set the texture file path that this billboard will use at runtime.
 * @param InTexturePath Filesystem or asset path to the texture to assign.
 */

/**
 * Get the quad mesh used to render the billboard.
 * @returns Pointer to the UQuad that represents the static mesh for this component, or `nullptr` if none.
 */

/**
 * Get the stored texture file path for this billboard.
 * @returns Reference to the texture file path string.
 */

/**
 * Retrieve the material assigned to the specified section index of this billboard.
 * @param InSectionIndex Section or element index to query.
 * @returns Pointer to the material interface used for the requested section, or `nullptr` if none.
 */

/**
 * Replace the material used by the specified element index of this billboard.
 * @param InElementIndex Element index whose material will be replaced.
 * @param InNewMaterial Material to assign to the specified element; may be `nullptr` to clear.
 */

/**
 * Perform post-serialization handling to restore or validate runtime-linked resources after this object has been serialized.
 */

/**
 * Duplicate sub-objects owned by this component when the component itself is duplicated.
 */
class UBillboardComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)
    GENERATED_REFLECTION_BODY()

    UBillboardComponent();
    ~UBillboardComponent() override = default;

    void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;

    // Setup
    void SetTextureName(FString InTexturePath);

    UQuad* GetStaticMesh() const { return Quad; }
    FString& GetFilePath() { return TexturePath; }

    UMaterialInterface* GetMaterial(uint32 InSectionIndex) const override;
    void SetMaterial(uint32 InElementIndex, UMaterialInterface* InNewMaterial) override;

    // Serialize
    void OnSerialized() override;

    // Duplication
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UBillboardComponent)

private:
    FString TexturePath;
    UTexture* Texture = nullptr;  // 리플렉션 시스템용 Texture 포인터
    UMaterialInterface* Material = nullptr;
    UQuad* Quad = nullptr;
};
