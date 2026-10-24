#include "VoxelSceneProxy.h"
#include "VoxelTerrainComponent.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "SceneManagement.h"

FVoxelSceneProxy::FVoxelSceneProxy(const UVoxelTerrainComponent* InComponent,
    UMaterialInterface* InMaterial)
    : FPrimitiveSceneProxy(InComponent)
{
    bWillEverBeLit = true;
    bCastDynamicShadow = true;
    bCastStaticShadow = false;

    UMaterialInterface* UseMat = InMaterial;
    if (!UseMat) UseMat = UMaterial::GetDefaultMaterial(MD_Surface);
    if (UseMat) MaterialProxy = UseMat->GetRenderProxy();
}

FVoxelSceneProxy::~FVoxelSceneProxy() {}

SIZE_T FVoxelSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

void FVoxelSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList) {}

void FVoxelSceneProxy::ReleaseChunkResources_RenderThread(FChunkRender& R)
{
    R.VertexFactory.ReleaseResource();
    R.PositionVB.ReleaseResource();
    R.StaticVB.ReleaseResource();
    R.ColorVB.ReleaseResource();
    R.IB.ReleaseResource();
    R.bReady = false;
}

void FVoxelSceneProxy::DestroyRenderThreadResources()
{
    for (auto& Pair : ChunkRenders)
    {
        if (Pair.Value.IsValid())
            ReleaseChunkResources_RenderThread(*Pair.Value);
    }
    ChunkRenders.Empty();
}

void FVoxelSceneProxy::AddChunk_RenderThread(
    FRHICommandListBase& RHICmdList,
    const FChunkKey& Key,
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> Mesh)
{
    // ⭐ 核心修复 3：如果是 LOD0，强制清理渲染端同坐标的 LOD1 保险
    if (Key.LOD == 0)
    {
        FChunkKey L1Key(Key.X, Key.Y, Key.Z, 1);
        if (auto* ExistingL1 = ChunkRenders.Find(L1Key))
        {
            if (ExistingL1->IsValid())
                ReleaseChunkResources_RenderThread(**ExistingL1);
            ChunkRenders.Remove(L1Key);
        }
    }

    if (!Mesh.IsValid()) return;

    const int32 NumVerts = Mesh->Positions.Num();
    const int32 NumIndices = Mesh->Indices.Num();
    if (NumVerts == 0 || NumIndices == 0) return;

    if (auto* Existing = ChunkRenders.Find(Key))
    {
        if (Existing->IsValid())
            ReleaseChunkResources_RenderThread(**Existing);
        ChunkRenders.Remove(Key);
    }

    auto R = MakeShared<FChunkRender>(GetScene().GetFeatureLevel());

    R->PositionVB.Init(NumVerts);
    R->StaticVB.Init(NumVerts, 1);
    R->ColorVB.Init(NumVerts);

    FBox ChunkBounds(ForceInit);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        const FVector3f& P = Mesh->Positions[i];
        R->PositionVB.VertexPosition(i) = P;

        const FVector3f N = Mesh->Normals.IsValidIndex(i) ? Mesh->Normals[i] : FVector3f(0, 0, 1);
        FVector3f T = FVector3f::CrossProduct(FVector3f(0, 0, 1), N);
        if (T.SizeSquared() < 1e-4f) T = FVector3f(1, 0, 0);
        T.Normalize();

        R->StaticVB.SetVertexTangents(i, T, FVector3f::CrossProduct(N, T), N);
        const FVector2f UV = Mesh->UVs.IsValidIndex(i) ? Mesh->UVs[i] : FVector2f(0, 0);
        R->StaticVB.SetVertexUV(i, 0, UV);
        R->ColorVB.VertexColor(i) = Mesh->Colors.IsValidIndex(i) ? Mesh->Colors[i] : FColor::White;

        ChunkBounds += FVector(P);
    }
    R->Bounds = ChunkBounds;

    R->IB.SetIndices(Mesh->Indices, EIndexBufferStride::Force32Bit);

    R->PositionVB.InitResource(RHICmdList);
    R->StaticVB.InitResource(RHICmdList);
    R->ColorVB.InitResource(RHICmdList);
    R->IB.InitResource(RHICmdList);

    FLocalVertexFactory::FDataType Data;
    R->PositionVB.BindPositionVertexBuffer(&R->VertexFactory, Data);
    R->StaticVB.BindTangentVertexBuffer(&R->VertexFactory, Data);
    R->StaticVB.BindPackedTexCoordVertexBuffer(&R->VertexFactory, Data);
    R->ColorVB.BindColorVertexBuffer(&R->VertexFactory, Data);
    R->VertexFactory.SetData(RHICmdList, Data);
    R->VertexFactory.InitResource(RHICmdList);

    R->bReady = true;
    ChunkRenders.Add(Key, R);
}

void FVoxelSceneProxy::RemoveChunk_RenderThread(
    FRHICommandListBase& RHICmdList,
    const FChunkKey& Key)
{
    if (auto* Existing = ChunkRenders.Find(Key))
    {
        if (Existing->IsValid())
            ReleaseChunkResources_RenderThread(**Existing);
        ChunkRenders.Remove(Key);
    }
}

void FVoxelSceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    uint32 VisibilityMap,
    FMeshElementCollector& Collector) const
{
    if (!MaterialProxy) return;
    if (ChunkRenders.Num() == 0) return;

    for (const auto& Pair : ChunkRenders)
    {
        const FChunkRender& R = *Pair.Value;
        if (!R.bReady) continue;
        if (R.IB.GetNumIndices() == 0) continue;

        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
        {
            if (!(VisibilityMap & (1 << ViewIndex))) continue;

            FMeshBatch& Mesh = Collector.AllocateMesh();
            Mesh.Elements.SetNum(1);
            FMeshBatchElement& Element = Mesh.Elements[0];

            Element.IndexBuffer = &R.IB;
            Element.FirstIndex = 0;
            Element.NumPrimitives = R.IB.GetNumIndices() / 3;
            Element.MinVertexIndex = 0;
            Element.MaxVertexIndex = R.PositionVB.GetNumVertices() - 1;
            Element.PrimitiveUniformBuffer = GetUniformBuffer();

            Mesh.VertexFactory = &R.VertexFactory;
            Mesh.MaterialRenderProxy = MaterialProxy;
            Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
            Mesh.Type = PT_TriangleList;
            Mesh.DepthPriorityGroup = SDPG_World;
            Mesh.bCanApplyViewModeOverrides = false;
            Mesh.bWireframe = false;

            Collector.AddMesh(ViewIndex, Mesh);
        }
    }
}

FPrimitiveViewRelevance FVoxelSceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance R;
    R.bDrawRelevance = true;
    R.bShadowRelevance = true;
    R.bDynamicRelevance = true;
    R.bStaticRelevance = false;
    R.bRenderInMainPass = true;
    R.bUsesLightingChannels = false;
    R.bOpaque = true;
    R.bEditorPrimitiveRelevance = false;
    return R;
}

uint32 FVoxelSceneProxy::GetMemoryFootprint() const
{
    uint32 Total = sizeof(*this);
    for (const auto& Pair : ChunkRenders)
    {
        if (!Pair.Value.IsValid()) continue;
        const FChunkRender& R = *Pair.Value;
        Total += R.PositionVB.GetNumVertices() * sizeof(FVector3f);
        Total += R.StaticVB.GetNumVertices() * 32;
        Total += R.IB.GetNumIndices() * sizeof(uint32);
    }
    return Total;
}