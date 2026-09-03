// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiDynamicLandscape.h"
#include "GamiDynamicNoise.h"
#include "DistanceFieldAtlas.h"


#include "StaticMeshOperations.h"
#include "MeshUtilities.h"
// Sets default values
ACxGamiDynamicLandscape::ACxGamiDynamicLandscape():APlanetActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

ACxGamiDynamicLandscape::~ACxGamiDynamicLandscape()
{

}

void ACxGamiDynamicLandscape::GetDistanceFieldFromStaticMesh(UStaticMesh* InSourceMesh, TArray<float>& InDensity, int32& InX, int32& InY, int32& InZ)
{

    IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

    // 直接用接口构建距离场
    TArray<FSparseDistanceFieldMip> OutMips;
    FBox3f Bounds;
    int32 NumMips = 1; // 只要最高精度


    FStaticMeshRenderData* RenderData = InSourceMesh->GetRenderData();
    if (!RenderData || RenderData->LODResources.Num() == 0) return;
    FMeshDataForDerivedDataTask MeshTaskData;
    MeshTaskData.SourceMeshData = nullptr;
    MeshTaskData.LODModel = &RenderData->LODResources[0];
    MeshTaskData.Bounds = (FBoxSphereBounds3f)InSourceMesh->GetBounds();
    TArray<FSignedDistanceFieldBuildSectionData> SectionDataArray;


    auto& LOD0 = RenderData->LODResources[0].Sections;
    for (int32 SectionIndex = 0; SectionIndex < (int32)LOD0.Num(); ++SectionIndex)
    {
        const auto& Section = LOD0[SectionIndex];

        FSignedDistanceFieldBuildSectionData SectionData;

        // 通过 MaterialIndex 从 UStaticMesh 取材质
        if (Section.MaterialIndex >= 0 && Section.MaterialIndex < SourceMesh->GetStaticMaterials().Num())
        {
            UMaterialInterface* Material = SourceMesh->GetStaticMaterials()[Section.MaterialIndex].MaterialInterface;
            if (Material)
            {
                SectionData.BlendMode = Material->GetBlendMode();
                SectionData.bTwoSided = Material->IsTwoSided();
            }
        }

        SectionData.bAffectDistanceFieldLighting = true;
        SectionDataArray.Add(SectionData);
    }
    MeshTaskData.SectionData = SectionDataArray;

    // Parts / Nodes 留空
    MeshTaskData.Parts = TConstArrayView<FSignedDistanceFieldBuildPartData>();
    MeshTaskData.Nodes = TConstArrayView<FNaniteAssemblyNode>();

    FDistanceFieldVolumeData OutData;

    // 调用生成
    MeshUtilities.GenerateSignedDistanceFieldVolumeData(
        SourceMesh->GetName(),
        MeshTaskData,
        1.0f,   // DistanceFieldResolutionScale，1.0 = 最高精度
        false,  // bGenerateAsIfTwoSided
        OutData
    );

    // 检查是否生成成功
    if (!OutData.IsValid())
    {
        return;
    }
    auto* data = (uint8*)OutData.StreamableMips.Lock(LOCK_READ_ONLY);


    const auto* Int32Table = (int32*)data;
    const auto& mips = OutData.Mips[0];
    int IndirectTableBytes = mips.IndirectionDimensions.X * mips.IndirectionDimensions.Y * mips.IndirectionDimensions.Z * 4;
    FBox3f Bounds2 = OutData.LocalSpaceMeshBounds;

    int32 DimX = mips.IndirectionDimensions.X * 8;
    int32 DimY = mips.IndirectionDimensions.Y * 8;
    int32 DimZ = mips.IndirectionDimensions.Z * 8;

    InDensity.Init(1.f , DimX * DimY * DimZ);
    InX = DimX;
    InY = DimY;
    InZ = DimZ;

    float VoxelSizeX = (Bounds2.Max.X - Bounds2.Min.X) / (float)DimX;
    float VoxelSizeY = (Bounds2.Max.Y - Bounds2.Min.Y) / (float)DimY;
    float VoxelSizeZ = (Bounds2.Max.Z - Bounds2.Min.Z) / (float)DimZ;
    float Scale = mips.DistanceFieldToVolumeScaleBias.X; // 不会是 0 了
    float Bias = mips.DistanceFieldToVolumeScaleBias.Y;
    for (int z = 0; z < mips.IndirectionDimensions.Z; z++)
    {
        for (int y = 0; y < mips.IndirectionDimensions.Y; y++)
        {
            for (int x = 0; x < mips.IndirectionDimensions.X; x++)
            {
                int TableIndex = z * (mips.IndirectionDimensions.X * mips.IndirectionDimensions.Y) + y * mips.IndirectionDimensions.X + x;

                if (Int32Table[TableIndex] == -1)
                {
                    continue;
                }
                int ChunkOffset = IndirectTableBytes + Int32Table[TableIndex] * 512;

                for (int subx = 0; subx < 8; subx++)
                {
                    for (int suby = 0; suby < 8; suby++)
                    {
                        for (int subz = 0; subz < 8; subz++)
                        {
                            //if (DrawnCount >= MaxDraw) break;

                            int VoxelIndex = subx + suby * 8 + subz * 64;
                            uint8 Raw = data[ChunkOffset + VoxelIndex];

                            // 反量化
                            float SignedDistance = (Raw / 255.f) * Scale + Bias;


                            // 全局体素坐标
                            int GX = x * 8 + subx;
                            int GY = y * 8 + suby;
                            int GZ = z * 8 + subz;

                            // 一维索引
                            int GlobalIndex = GX + GY * DimX + GZ * DimX * DimY;
                            InDensity[GlobalIndex] = SignedDistance;

                            // 计算位置
                      /*      FVector Pos(
                                Bounds2.Min.X + (x * 8 + subx + 0.5f) * VoxelSizeX,
                                Bounds2.Min.Y + (y * 8 + suby + 0.5f) * VoxelSizeY,
                                Bounds2.Min.Z + (z * 8 + subz + 0.5f) * VoxelSizeZ
                            );*/

                            // 红=内部，蓝=外部
                        /*    FColor Color = SignedDistance < 0.0f ? FColor::Red : FColor::Blue;
                            if (Color == FColor::Blue)
                            {
                                continue;
                            }*/
                            //DrawDebugSphere(GetWorld(), Pos, 1.f, 4, Color, true, -1.0f, 0, 0.5f);
                            //DrawnCount++;
                        }
                    }
                }
            }
        }
    }
    OutData.StreamableMips.Unlock();

}

// Called when the game starts or when spawned
void ACxGamiDynamicLandscape::BeginPlay()
{
	Super::BeginPlay();





    TArray<float> Density;
    int32 X = 0;
    int32 Y = 0;
    int32 Z = 0;
    GetDistanceFieldFromStaticMesh(this->SourceMesh, Density, X, Y, Z);
    FVector ChunkCenter(0, 0, 0);
    APlanetActor::GenerateChunk(0, 0, 0, ChunkCenter,  FVector(X, Y, Z), Density, FVector(X, Y, Z));

#if 0
    for (int i = 0; i < X; i++)
    {
        for (int j = 0; j < Y; j++)
        {
            for (int k = 0; k < Z; k++)
            {
                int index = k * (X * Y) + j * X + i;
                if (Density[index] >= 0.f)
                    continue;
                FColor Color = Density[index] < 0.f ? FColor::Green : FColor::Red;
                FVector Pos(i, j, k);
                Pos = Pos * 10.f;
                DrawDebugSphere(GetWorld(), Pos, 4.f, 4, Color, true, -1.0f, 0, 0.5f);
                
            }
        }
    }
#endif



}

// Called every frame
void ACxGamiDynamicLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
