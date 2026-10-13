#pragma once
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "LocalVertexFactory.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/StaticMeshVertexBuffer.h"
#include "Rendering/ColorVertexBuffer.h"
#include "RawIndexBuffer.h"
#include "VoxelMeshData.h"
#include "VoxelTypes.h"

class UVoxelTerrainComponent;

class VOXELTERRAIN_API FVoxelSceneProxy : public FPrimitiveSceneProxy
{
public:
    /** 单个 chunk 的渲染资源 —— 每个 chunk 有一份独立的 VB/IB/VF */
    struct FChunkRender
    {
        FPositionVertexBuffer   PositionVB;
        FStaticMeshVertexBuffer StaticVB;
        FColorVertexBuffer      ColorVB;
        FRawStaticIndexBuffer   IB;
        FLocalVertexFactory     VertexFactory;
        FBox                    Bounds = FBox(ForceInit);
        bool                    bReady = false;

        explicit FChunkRender(ERHIFeatureLevel::Type FL)
            : VertexFactory(FL, "FVoxelChunkVF") {
        }
    };

    FVoxelSceneProxy(const UVoxelTerrainComponent* InComponent,
        UMaterialInterface* InMaterial);
    virtual ~FVoxelSceneProxy() override;

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

    /** ⭐ 增量接口：加/删一个 chunk（只能在 RT 调用） */
    void AddChunk_RenderThread(
        FRHICommandListBase& RHICmdList,
        const FChunkKey& Key,
        TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh);

    void RemoveChunk_RenderThread(
        FRHICommandListBase& RHICmdList,
        const FChunkKey& Key);

private:
    void ReleaseChunkResources_RenderThread(FChunkRender& R);

    const FMaterialRenderProxy* MaterialProxy = nullptr;

    // Key → chunk 的 GPU 资源
    // 只在 RT 上访问，不需要线程安全
    TMap<FChunkKey, TSharedPtr<FChunkRender>> ChunkRenders;
};