// Fill out your copyright notice in the Description page of Project Settings.

#include "CxDynamicLandScape.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>
#include "Math/UnrealMathUtility.h"
#include "StaticMeshOperations.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "Components/LightComponent.h"

float SampleDensity(float WorldX, float WorldY, float WorldZ)
{
	const float Frequency = 0.005f;
	const float Amplitude = 1000.0f;
	const float BaseHeight = 200.0f;

	float noise = FMath::PerlinNoise2D(FVector2D(
		WorldX * Frequency,
		WorldY * Frequency
	));

	float surfaceHeight = BaseHeight + noise * Amplitude;
	return WorldZ - surfaceHeight;
}

ACxDynamicLandScape::ACxDynamicLandScape()
{
	PrimaryActorTick.bCanEverTick = true;
	ProduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProduralMeshComponent;

}

void ACxDynamicLandScape::BeginPlay()
{
	Super::BeginPlay();
	this->BrutusGrid = new Brutus::Grid(this->LandscapeDimension.X, this->LandscapeDimension.Y, this->LandscapeDimension.Z);
}

void ACxDynamicLandScape::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ACxDynamicLandScape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
void ACxDynamicLandScape::CreateDensityField(int32 Padding)
{
	Brutus::Grid& grid = *this->BrutusGrid;
	Brutus::Size3D total = grid.total_size();  // 64, 64, 64
	const FVector worldOffset = GetActorLocation();
	const float noiseHeight = 200.f;
	const float noiseFreq = 0.2f;
	const float BaseHeight = 200.f;
	float DensityMin = -BaseHeight - noiseHeight;
	float DensityMax = ((total.z - 1) * VoxelSize) - BaseHeight + noiseHeight;
	for (int32 z = Padding; z < (int32)total.z - Padding; ++z)
	{
		for (int32 y = Padding; y < (int32)total.y - Padding; ++y)
		{
			for (int32 x = Padding; x < (int32)total.x - Padding; ++x)
			{
				float sampleX = (worldOffset.X / VoxelSize) + x;
				float sampleY = (worldOffset.Y / VoxelSize) + y;
				float sampleZ = (worldOffset.Z / VoxelSize) + z;

				sampleX *= noiseFreq;
				sampleY *= noiseFreq;
				sampleZ *= noiseFreq;
#if 0
				float noiseValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * noiseHeight;
				float Density = (z * this->VoxelSize) - (BaseHeight + noiseValue);

				float Weight = FMath::GetMappedRangeValueClamped(
					FVector2D(DensityMin, DensityMax),
					FVector2D(-127.0f, 127.0f),
					Density
				);

				grid(x, y, z).weight = Weight;
#else

				// 3D Perlin 噪声（UE 内置）
				// 返回范围 [-1, 1]，乘以 noiseHeight 得到高度偏移
				float noiseValue = FMath::PerlinNoise3D(FVector(sampleX, sampleY, sampleZ)) * noiseHeight;

				// 计算密度值：z轴高度减去基础高度和噪声偏移
				float Density = (z * this->VoxelSize) - (BaseHeight + noiseValue);

				// 映射到权重范围 [-127, 127]
				float Weight = FMath::GetMappedRangeValueClamped(
					FVector2D(DensityMin, DensityMax),
					FVector2D(-127.0f, 127.0f),
					Density
				);

				grid(x, y, z).weight = Weight;
#endif
			}
		}
	}
}
void ACxDynamicLandScape::SetDensityField(int32 x, int32 y, int32 z, int Weight)
{
	Brutus::Grid& grid = *this->BrutusGrid;
	grid(x, y, z).weight = Weight;
}
void ACxDynamicLandScape::UpdateMeshFromDensityMC()
{
	// ========== 1. 创建 Grid ==========
	// Grid 的构造函数参数是 CHUNK 数量，不是体素数量
	// 默认 BRUTUS_CHUNK_SIZE = 8，所以 4x4x4 个 chunk = 32x32x32 体素
	const int32 chunksX = LandscapeDimension.X;
	const int32 chunksY = LandscapeDimension.Y;
	const int32 chunksZ = LandscapeDimension.Z;
	Brutus::Grid& grid = *this->BrutusGrid;
	grid.DeleteVoxel = this->DeleteVoxel;

	// ========== 2. 填充密度场（临时，函数结束自动释放） 
	// 密度场会另外提供接口生成
#if 0
	Brutus::Size3D total = grid.total_size();  // 64, 64, 64
	const FVector worldOffset = GetActorLocation();
	const float noiseHeight = 400.f;
	const float noiseFreq = 0.2f;
	const float BaseHeight = 200.f;
	float DensityMin = -BaseHeight - noiseHeight;
	float DensityMax = ((total.z - 1) * VoxelSize) - BaseHeight + noiseHeight;
	for (int32 z = 0; z < (int32)total.z; ++z)
	{
		for (int32 y = 0; y < (int32)total.y; ++y)
		{
			for (int32 x = 0; x < (int32)total.x; ++x)
			{
				float sampleX = (worldOffset.X / VoxelSize) + x;
				float sampleY = (worldOffset.Y / VoxelSize) + y;
				float sampleZ = (worldOffset.Z / VoxelSize) + z;

				sampleX *= noiseFreq;
				sampleY *= noiseFreq;
				sampleZ *= noiseFreq;
				float noiseValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * noiseHeight;
				float Density = (z * this->VoxelSize) - (BaseHeight + noiseValue);

				float Weight = FMath::GetMappedRangeValueClamped(
					FVector2D(DensityMin, DensityMax),
					FVector2D(-127.0f, 127.0f),
					Density
				);

				grid(x, y, z).weight = Weight;
			}
		}
	}
#endif


	// ========== 3. 提取所有 chunk 的网格 ==========
	TArray<FVector> OutVerts;
	TArray<int32> OutTris;
	TArray<FVector> OutNormals;
	TArray<FVector2D> uvs;
	TArray<FColor> vertexColors;
	TArray<FProcMeshTangent> tangents;

	int32 vertexOffset = 0;

	for (int32 cz = 0; cz < chunksZ; ++cz)
	{
		for (int32 cy = 0; cy < chunksY; ++cy)
		{
			for (int32 cx = 0; cx < chunksX; ++cx)
			{
				// 生成这个 chunk 的网格
				Brutus::Mesh mesh = grid.generate_mesh(cx, cy, cz);

				if (mesh.vertex_count == 0) continue;

				// 顶点
				for (size_t i = 0; i < mesh.vertex_count; ++i)
				{
					OutVerts.Add(FVector(
						mesh.vertices[i * 3] * VoxelSize,
						mesh.vertices[i * 3 + 1] * VoxelSize,
						mesh.vertices[i * 3 + 2] * VoxelSize
					));

					if (mesh.normals)
					{
						OutNormals.Add(FVector(
							mesh.normals[i * 3],
							mesh.normals[i * 3 + 1],
							mesh.normals[i * 3 + 2]
						));
					}
				}

		
				for (size_t i = 0; i < mesh.vertex_count; i += 3)
				{
					OutTris.Add(vertexOffset + static_cast<int32>(i + 0));
					OutTris.Add(vertexOffset + static_cast<int32>(i + 2)); // 交换 1 和 2
					OutTris.Add(vertexOffset + static_cast<int32>(i + 1));
				}

				vertexOffset += static_cast<int32>(mesh.vertex_count);

				// mesh 析构时会自动释放 vertices/normals 内存
			}
		}
	}

	// UV
	uvs.SetNum(OutVerts.Num());
	for (int32 i = 0; i < OutVerts.Num(); ++i)
	{
		uvs[i] = FVector2D(OutVerts[i].X * 0.005f, OutVerts[i].Y * 0.005f);
	}

	// ========== 4. 创建网格 ==========
	if (OutVerts.Num() > 0)
	{
		ProduralMeshComponent->CreateMeshSection(0, OutVerts, OutTris, OutNormals, uvs, vertexColors, tangents, true);
		if (VoxelMaterial)
		{
			ProduralMeshComponent->SetMaterial(0, VoxelMaterial);
		}
	}
}

FDensityInformation ACxDynamicLandScape::GetDensityX(int32 x)
{
	if (!BrutusGrid)
		return FDensityInformation();

	FDensityInformation result;

	Brutus::Grid& grid = *this->BrutusGrid;///(chunksX, chunksY, chunksZ);
	auto total = grid.total_size();
	result.Density.Init(1.f, total.y * total.z);
	result.Position.Init(FVector(0.f), total.y * total.z);
	for (int32 z = 0; z < (int32)total.z; z++)
	{
		for (int32 y = 0; y < (int32)total.y; y++)
		{
			int index = z * total.y + y;
			result.Density[index] = grid(x, y, z).weight;
			result.Position[index] = this->GetActorLocation() + FVector(x * this->VoxelSize, y * this->VoxelSize, z * this->VoxelSize);
		}
	}

	return result;
}

FDensityInformation ACxDynamicLandScape::GetDensity()
{
	if (!BrutusGrid)
		return FDensityInformation();

	FDensityInformation result;

	Brutus::Grid& grid = *this->BrutusGrid;///(chunksX, chunksY, chunksZ);
	auto total = grid.total_size();
	result.Density.Init(1.f, total.x * total.y * total.z);
	result.Position.Init(FVector(0.f), total.x * total.y * total.z);
	for (int32 z = 0; z < (int32)total.z; z++)
	{
		for (int32 y = 0; y < (int32)total.y; y++)
		{
			for (int x = 0; x < (int32)total.x; x++)
			{
				int index = (z * total.x * total.y) + y * total.x + x;
				result.Density[index] = grid(x, y, z).weight;
				result.Position[index] = this->GetActorLocation() + FVector(x * this->VoxelSize, y * this->VoxelSize, z * this->VoxelSize);
			}
		}
	}

	return result;
}

FDensityInformation ACxDynamicLandScape::GetLODDensity(int32 InX, int32 InLodLevel, FString InLogName)
{
	FDensityInformation result;
	Brutus::Grid& grid = *this->BrutusGrid;///(chunksX, chunksY, chunksZ);
	auto total = grid.total_size();
	result.Density.Init(1.f, total.x * total.y * total.z);
	result.Position.Init(FVector(0.f), total.y * total.z);

	FString FilePath = FPaths::ProjectSavedDir() + InLogName;
	FString OutputString;
	OutputString.Reserve(total.y * total.z * 10); // 预分配内存提高性能

	for (int z = 0; z < total.z; z += 2)
	{
		for (int y = 0; y < total.y; y += 2)
		{
			auto n0 = grid(InX, y, z).weight;
			auto n1 = grid(InX, y, z + 2).weight;
			auto n2 = grid(InX, y + 2, z).weight;
			auto n3 = grid(InX, y + 2, z + 2).weight;

			
			
		}
		OutputString += TEXT("\n");
	}
	OutputString += TEXT("\n");

	FFileHelper::SaveStringToFile(OutputString, *FilePath);
	return result;
}

void ACxDynamicLandScape::DebugWriteArray2Desk(FString InLogName)
{
	Brutus::Grid& grid = *this->BrutusGrid;

	// 获取Grid尺寸
	const int32 ChunksX = grid.total_size().x;
	const int32 ChunksY = grid.total_size().y;
	const int32 ChunksZ = grid.total_size().z;

	// 构建文件路径（保存到项目Saved文件夹）
	FString FilePath = FPaths::ProjectSavedDir() + InLogName;

	// 构建输出内容
	FString OutputString;
	OutputString.Reserve(ChunksX * ChunksY * ChunksZ * 10); // 预分配内存提高性能

	for (int32 X = 0; X < ChunksX; ++X)
	{
		OutputString += FString::Printf(TEXT("=== x=%d ===\n"), X);

		for (int32 Y = 0; Y < ChunksY; ++Y)
		{
			for (int32 Z = 0; Z < ChunksZ; ++Z)
			{
				float WeightValue = grid(X, Y, Z).weight;
				OutputString += FString::Printf(TEXT("%.2f "), WeightValue);
			}
			OutputString += TEXT("\n");
		}
		OutputString += TEXT("\n");
	}

	FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

