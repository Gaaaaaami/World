#include "VoxelSceneProxy.h"
#include "VoxelTerrainComponent.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "SceneManagement.h"

FVoxelSceneProxy::FVoxelSceneProxy(const UVoxelTerrainComponent* InComponent,
    UMaterialInterface* InMaterial)
    : FPrimitiveSceneProxy(InComponent)
    , VertexFactory(GetScene().GetFeatureLevel(), "FVoxelSceneProxy")
{
    bWillEverBeLit = true;
    bCastDynamicShadow = true;
    bCastStaticShadow = false;

    // 用传入的材质；如果没有，退回到默认材质
    UMaterialInterface* UseMat = InMaterial;
    if (!UseMat)
        UseMat = UMaterial::GetDefaultMaterial(MD_Surface);

    if (UseMat)
        MaterialProxy = UseMat->GetRenderProxy();
}
SIZE_T FVoxelSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

void FVoxelSceneProxy::CreateRenderThreadResources(FRHICommandListBase& RHICmdList)
{
    // UE5.8 要求 FLocalVertexFactory 的 position SRV 非空。
    // 用一个 dummy 顶点占位，等到真正数据来（UploadMeshData_RenderThread）再重建。
    PositionVertexBuffer.Init(1);
    PositionVertexBuffer.VertexPosition(0) = FVector3f(0, 0, 0);

    StaticMeshVertexBuffer.Init(1, 1);
    StaticMeshVertexBuffer.SetVertexTangents(
        0, FVector3f(1, 0, 0), FVector3f(0, 1, 0), FVector3f(0, 0, 1));
    StaticMeshVertexBuffer.SetVertexUV(0, 0, FVector2f(0, 0));

    ColorVertexBuffer.Init(1);
    ColorVertexBuffer.VertexColor(0) = FColor::White;

    IndexBuffer.SetIndices(TArray<uint32>(), EIndexBufferStride::Force32Bit);

    PositionVertexBuffer.InitResource(RHICmdList);
    StaticMeshVertexBuffer.InitResource(RHICmdList);
    ColorVertexBuffer.InitResource(RHICmdList);
    IndexBuffer.InitResource(RHICmdList);

    FLocalVertexFactory::FDataType Data;
    PositionVertexBuffer.BindPositionVertexBuffer(&VertexFactory, Data);
    StaticMeshVertexBuffer.BindTangentVertexBuffer(&VertexFactory, Data);
    StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&VertexFactory, Data);
    ColorVertexBuffer.BindColorVertexBuffer(&VertexFactory, Data);
    VertexFactory.SetData(RHICmdList, Data);
    VertexFactory.InitResource(RHICmdList);

    bResourcesReady = true;
}
void FVoxelSceneProxy::DestroyRenderThreadResources()
{
    bResourcesReady = false;

    VertexFactory.ReleaseResource();
    PositionVertexBuffer.ReleaseResource();
    StaticMeshVertexBuffer.ReleaseResource();
    ColorVertexBuffer.ReleaseResource();
    IndexBuffer.ReleaseResource();
}

void FVoxelSceneProxy::UploadMeshData_RenderThread(
    FRHICommandListBase& RHICmdList,
    TSharedPtr<FVoxelMeshData, ESPMode::ThreadSafe> InMeshData)
{
    if (!InMeshData.IsValid()) return;

    const int32 NumVerts = InMeshData->Positions.Num();
    const int32 NumIndices = InMeshData->Indices.Num();
    if (NumVerts == 0 || NumIndices == 0) return;

    // ---- 1) 释放旧资源 ----
    VertexFactory.ReleaseResource();
    PositionVertexBuffer.ReleaseResource();
    StaticMeshVertexBuffer.ReleaseResource();
    ColorVertexBuffer.ReleaseResource();
    IndexBuffer.ReleaseResource();

    // ---- 2) 分配新缓冲 ----
    PositionVertexBuffer.Init(NumVerts);
    StaticMeshVertexBuffer.Init(NumVerts, 1);
    ColorVertexBuffer.Init(NumVerts);

    for (int32 i = 0; i < NumVerts; ++i)
    {
        const FVector3f& P = InMeshData->Positions[i];
        PositionVertexBuffer.VertexPosition(i) = P;

        // 使用网格数据里的真实法线
        const FVector3f N = InMeshData->Normals.IsValidIndex(i)
            ? InMeshData->Normals[i]
            : FVector3f(0, 0, 1);

        // 从法线算一组正交切线基（简单版：Z 轴投影）
        FVector3f T = FVector3f::CrossProduct(FVector3f(0, 0, 1), N);
        if (T.SizeSquared() < 1e-4f) T = FVector3f(1, 0, 0);
        T.Normalize();
        const FVector3f B = FVector3f::CrossProduct(N, T);

        StaticMeshVertexBuffer.SetVertexTangents(i, T, B, N);
        StaticMeshVertexBuffer.SetVertexUV(i, 0, FVector2f(P.X * 0.01f, P.Y * 0.01f));
        ColorVertexBuffer.VertexColor(i) = FColor::White;
    }
    IndexBuffer.SetIndices(InMeshData->Indices, EIndexBufferStride::Force32Bit);

    // ---- 3) 上传 RHI ----
    PositionVertexBuffer.InitResource(RHICmdList);
    StaticMeshVertexBuffer.InitResource(RHICmdList);
    ColorVertexBuffer.InitResource(RHICmdList);
    IndexBuffer.InitResource(RHICmdList);

    // ---- 4) 绑定到 VertexFactory ----
    FLocalVertexFactory::FDataType Data;
    PositionVertexBuffer.BindPositionVertexBuffer(&VertexFactory, Data);
    StaticMeshVertexBuffer.BindTangentVertexBuffer(&VertexFactory, Data);
    StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&VertexFactory, Data);
    ColorVertexBuffer.BindColorVertexBuffer(&VertexFactory, Data);
    VertexFactory.SetData(RHICmdList, Data);
    VertexFactory.InitResource(RHICmdList);

    bResourcesReady = true;
}

void FVoxelSceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    uint32 VisibilityMap,
    FMeshElementCollector& Collector) const
{
    if (!bResourcesReady) return;
    if (IndexBuffer.GetNumIndices() == 0) return;
    if (!MaterialProxy) return;

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
    {
        if (!(VisibilityMap & (1 << ViewIndex))) continue;

        FMeshBatch& Mesh = Collector.AllocateMesh();
        Mesh.Elements.SetNum(1);
        FMeshBatchElement& Element = Mesh.Elements[0];

        Element.IndexBuffer = &IndexBuffer;
        Element.FirstIndex = 0;
        Element.NumPrimitives = IndexBuffer.GetNumIndices() / 3;
        Element.MinVertexIndex = 0;
        Element.MaxVertexIndex = PositionVertexBuffer.GetNumVertices() - 1;
        Element.PrimitiveUniformBuffer = GetUniformBuffer();

        Mesh.bWireframe = false;
        Mesh.VertexFactory = &VertexFactory;
        Mesh.MaterialRenderProxy = MaterialProxy;
        Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
        Mesh.Type = PT_TriangleList;
        Mesh.DepthPriorityGroup = SDPG_World;
        Mesh.bCanApplyViewModeOverrides = false;

        Collector.AddMesh(ViewIndex, Mesh);
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
    return sizeof(*this)
        + PositionVertexBuffer.GetNumVertices() * sizeof(FVector3f)
        + StaticMeshVertexBuffer.GetNumVertices() * 32
        + IndexBuffer.GetNumIndices() * sizeof(uint32);
}