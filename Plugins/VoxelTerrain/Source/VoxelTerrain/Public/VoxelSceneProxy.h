#pragma once
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "LocalVertexFactory.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/StaticMeshVertexBuffer.h"
#include "Rendering/ColorVertexBuffer.h"
#include "RawIndexBuffer.h"
#include "VoxelMeshData.h"

class UVoxelTerrainComponent;
class UMaterialInterface;

class VOXELTERRAIN_API FVoxelSceneProxy : public FPrimitiveSceneProxy
{
public:
    FVoxelSceneProxy(const UVoxelTerrainComponent* InComponent,
        UMaterialInterface* InMaterial);

    virtual SIZE_T GetTypeHash() const override;
    virtual void   CreateRenderThreadResources(FRHICommandListBase& RHICmdList) override;
    virtual void   DestroyRenderThreadResources() override;
    virtual void   GetDynamicMeshElements(
        const TArray<const FSceneView*>& Views,
        const FSceneViewFamily& ViewFamily,
        uint32 VisibilityMap,
        FMeshElementCollector& Collector) const override;
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
    virtual uint32 GetMemoryFootprint() const override;

    void UploadMeshData_RenderThread(
        FRHICommandListBase& RHICmdList,
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> InMeshData);

private:
    FPositionVertexBuffer   PositionVertexBuffer;
    FStaticMeshVertexBuffer StaticMeshVertexBuffer;
    FColorVertexBuffer      ColorVertexBuffer;
    FRawStaticIndexBuffer   IndexBuffer;
    FLocalVertexFactory     VertexFactory;

    FMaterialRenderProxy* MaterialProxy = nullptr;
    bool bResourcesReady = false;
};