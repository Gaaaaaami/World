#include "CxDynamicLandScapeManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

ACxDynamicLandScapeManager::ACxDynamicLandScapeManager()
{
	PrimaryActorTick.bCanEverTick = false; // 不需要每帧跑
}

void ACxDynamicLandScapeManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnAllChunks();
}

void ACxDynamicLandScapeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/* ===================== 私有：生成与坐标工具 ===================== */

FIntPoint ACxDynamicLandScapeManager::WorldToGridCoord(const FVector& WorldLoc) const
{
	float StrideX = ChunkDimX * GlobalVoxelSize;
	float StrideY = ChunkDimY * GlobalVoxelSize;
	return FIntPoint(
		FMath::FloorToInt(WorldLoc.X / StrideX),
		FMath::FloorToInt(WorldLoc.Y / StrideY)
	);
}

FVector ACxDynamicLandScapeManager::WorldToLocalVoxel(const ACxDynamicLandScape* Chunk, const FVector& WorldLoc) const
{
	if (!Chunk) return FVector::ZeroVector;
	// 直接用 Actor 逆变换，再除以体素大小得到浮点体素坐标
	FVector Local = Chunk->GetActorTransform().InverseTransformPosition(WorldLoc);
	return Local / GlobalVoxelSize;
}
void ACxDynamicLandScapeManager::SpawnAllChunks()
{
	// 不要用 (Dim-1)，老老实实按真实体积摆
	float ChunkStrideX = (ChunkDimX - 2) * GlobalVoxelSize;
	float ChunkStrideY = (ChunkDimY - 2) * GlobalVoxelSize;

	int32 TotalHalf = GridRadius;
	int m = 0;
	for (int32 i = -TotalHalf; i <= TotalHalf; ++i)
	{
		for (int32 j = -TotalHalf; j <= TotalHalf; ++j)
		{
			// 真实步长摆放
			FVector Location(i * ChunkStrideX, j * ChunkStrideY, 0.f);
			FTransform SpawnTM(FRotator::ZeroRotator, Location);
			ACxDynamicLandScape* NewChunk = GetWorld()->SpawnActor<ACxDynamicLandScape>(ACxDynamicLandScape::StaticClass(), SpawnTM);
			if (NewChunk)
			{
				NewChunk->SetOwner(this);			
				NewChunk->LandscapeDimension = FVector(ChunkDimX, ChunkDimY, ChunkDimZ);
				NewChunk->VoxelSize = GlobalVoxelSize;

				if (VoxelMaterialRef) 
					NewChunk->VoxelMaterial = VoxelMaterialRef;

				NewChunk->GenerateDensityField(NewChunk->LandscapeDimension);
				NewChunk->UpdateMeshFromDensity();
				ChunkGrid.Add(FIntPoint(i, j), NewChunk);
			
			}

			m++;

		}
	}
}
/* ===================== 核心：跨 Chunk 挖洞 ===================== */

/* ===================== 核心：跨 Chunk 挖洞（安全版）===================== */

// 辅助：获取某个 Chunk 指定局部坐标的密度引用（安全）
static float& GetDensity(TArray<float>& Data, int32 DimX, int32 DimY, int32 DimZ, FIntVector Coord)
{
	static float Dummy = 0;
	if (Coord.X < 0 || Coord.X >= DimX || Coord.Y < 0 || Coord.Y >= DimY || Coord.Z < 0 || Coord.Z >= DimZ) return Dummy;
	int32 idx = Coord.Z * (DimX * DimY) + Coord.Y * DimX + Coord.X;
	return Data.IsValidIndex(idx) ? Data[idx] : Dummy;
}

void ACxDynamicLandScapeManager::GlobalDigAtLocation(FVector WorldLocation, float BrushRadiusCm, float Strength)
{
	if (ChunkGrid.Num() == 0) return;

	const float RadiusInVoxels = FMath::Max(1.0f, BrushRadiusCm / GlobalVoxelSize);
	FIntPoint CenterCoord = WorldToGridCoord(WorldLocation);

	// 稍微多溢出一个 Chunk，让洞能跨过接缝
	int32 AffectDist = FMath::CeilToInt(RadiusInVoxels / (ChunkDimX - 1)) + 1;

	TArray<ACxDynamicLandScape*> NeedUpdateChunks; // 用 Array 代替 TSet，避免之前的问题

	for (int32 dx = -AffectDist; dx <= AffectDist; ++dx)
	{
		for (int32 dy = -AffectDist; dy <= AffectDist; ++dy)
		{
			FIntPoint Key = CenterCoord + FIntPoint(dx, dy);
			ACxDynamicLandScape** Found = ChunkGrid.Find(Key);
			if (!Found || !IsValid(*Found)) continue;

			ACxDynamicLandScape* Chunk = *Found;
			if (Chunk->m_DensityData.Num() == 0) continue;

			// 转为该 Chunk 局部浮点体素坐标
			FVector CenterVoxel = WorldToLocalVoxel(Chunk, WorldLocation);
			int32 CX = FMath::RoundToInt(CenterVoxel.X);
			int32 CY = FMath::RoundToInt(CenterVoxel.Y);
			int32 CZ = FMath::RoundToInt(CenterVoxel.Z);
			int32 R = FMath::CeilToInt(RadiusInVoxels);

			// 只在本 Chunk 范围内自然挖，不越俎代庖也不强行同步
			for (int32 z = FMath::Max(0, CZ - R); z <= FMath::Min(ChunkDimZ - 1, CZ + R); ++z)
				for (int32 y = FMath::Max(0, CY - R); y <= FMath::Min(ChunkDimY - 1, CY + R); ++y)
					for (int32 x = FMath::Max(0, CX - R); x <= FMath::Min(ChunkDimX - 1, CX + R); ++x)
					{
						float Dist = FVector::Dist(FVector(x, y, z), CenterVoxel);
						if (Dist > RadiusInVoxels) continue;

						float Falloff = FMath::Pow(1.0f - (Dist / RadiusInVoxels), 1.5f);
						int32 idx = z * (ChunkDimX * ChunkDimY) + y * ChunkDimX + x;
						if (Chunk->m_DensityData.IsValidIndex(idx))
						{
							Chunk->m_DensityData[idx] = FMath::Max(Chunk->m_DensityData[idx], Falloff * Strength * 2.0f);
						}
					}

			NeedUpdateChunks.AddUnique(Chunk);
		}
	}

	// ---- 统一刷新，不玩花活 ----
	for (auto* C : NeedUpdateChunks)
	{
		if (C) C->UpdateMeshFromDensity();
	}
}