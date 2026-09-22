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
#include "Transvoxel.h"
#include "DebugTransvoxel.h"



//#define DEBUG_GAMI_CHUNK


ACxDynamicLandScape::ACxDynamicLandScape()
{
	PrimaryActorTick.bCanEverTick = true;
	ProduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProduralMeshComponent;
	PlayerLocation = FVector(0);
}

void ACxDynamicLandScape::BeginPlay()
{
	Super::BeginPlay();
	this->LoadedRangeSqared = this->LoadedRange * this->LoadedRange;
	
#if 0
	FChunkManagerInformation ChunkManagerInformation;
	ChunkManagerInformation.LandscapeDimension = this->LandscapeDimension;
	ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X,
		ChunkManagerInformation.LandscapeDimension.Y,
		ChunkManagerInformation.LandscapeDimension.Z);
	ChunkManagerInformation.VoxelSize = this->VoxelSize;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 2;
	ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 1;

	ChunkManagerInformation.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation.MeshComponent->RegisterComponent();
	ChunkManagerInformation.MeshComponent->SetWorldLocation(FVector(0, 0, 0));
	ChunkManagerInformation.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation.MeshComponent->Location = FVector(0);
	ChunkManagerInformation.MeshComponent->LOD = 0;
	//LOD1.Add(ChunkManagerInformation);
	//CreateDensityField(ChunkManagerInformation);
	//UpdateMeshFromDensityMC(ChunkManagerInformation);

	Brutus::Size3D total = { this->LandscapeDimension.X * BRUTUS_CHUNK_SIZE,
							 this->LandscapeDimension.Y * BRUTUS_CHUNK_SIZE,
							 this->LandscapeDimension.Z * BRUTUS_CHUNK_SIZE };
	auto DensityInformation =  GetDensityX(total.x - 1, ChunkManagerInformation);
	
	TArray<FVector> TransitionVertices;
	TArray<int32> TransitionTriangle;
	TArray<float> LowRes;
	TArray<FVector> LowPos;

	CreateTransitionMesh(
		total,
		FVector(1, 0, 0),
		FVector(0, 0, 0),
		100.f,
		ChunkManagerInformation.VoxelSize,
		TransitionVertices,
		TransitionTriangle
	);

	//ADebugTransvoxel* DebugTransvoxel = NewObject<ADebugTransvoxel>(this);
	//DebugTransvoxel->BuildTransition_MultiUnit(TransitionVertices, TransitionTriangle, DensityInformation.Density,
	//	LowRes, DensityInformation.Position, LowPos, FVector(15, 15, 15),
	//	FVector(7, 7, 7), FVector(1, 0, 0), 100.f, ChunkManagerInformation.VoxelSize);
	TArray<FVector> Normals;
	TArray<FVector2D> UVS;
	TArray<FColor> Color;
	TArray<FProcMeshTangent> Tangents;
	ChunkManagerInformation.MeshComponent->CreateMeshSection(1,
		TransitionVertices,
		TransitionTriangle,
		Normals,
		UVS,
		Color,
		Tangents,
		false);
	UE_LOG(LogTemp, Log, TEXT("Vertices Nums : %d, Triangles Nums : %d"), TransitionVertices.Num(), TransitionTriangle.Num());



	


#if 1
	FChunkManagerInformation ChunkManagerInformation0;

	ChunkManagerInformation0.LandscapeDimension = (this->LandscapeDimension / 2) + FVector(1, 1, 0);
	ChunkManagerInformation0.BrutusGrid = new Brutus::Grid(ChunkManagerInformation0.LandscapeDimension.X,
		ChunkManagerInformation0.LandscapeDimension.Y,
		ChunkManagerInformation0.LandscapeDimension.Z);
	ChunkManagerInformation0.VoxelSize = this->VoxelSize * 2;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation0.BrutusGrid->XEndDeleteVoxel = 4;
	ChunkManagerInformation0.BrutusGrid->YEndDeleteVoxel = 4;

	ChunkManagerInformation0.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation0.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation0.MeshComponent->RegisterComponent();
	ChunkManagerInformation0.MeshComponent->SetWorldLocation(FVector(1500, 0, 0));
	ChunkManagerInformation0.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation0.MeshComponent->Location = FVector(1500, 0, 0);

	ChunkManagerInformation0.MeshComponent->LOD = 1;
	//LOD1.Add(ChunkManagerInformation);

	CreateDensityField(ChunkManagerInformation0);
	UpdateMeshFromDensityMC(ChunkManagerInformation0);
#endif

	FVector Noise0Location(0);
	float Noise0 = GetNoise(15, 0, 0, total, Noise0Location, 100.f, 0);
	float Noise1 = GetNoise(FVector(0, 0, 0), total, ChunkManagerInformation0.MeshComponent->Location, 100.f, 1);


#endif

}
FDensityInformation ACxDynamicLandScape::GetDensityX(int32 InX, const FChunkManagerInformation& InChunkManagerInformation)
{
	FDensityInformation result;
	Brutus::Grid& grid = *InChunkManagerInformation.BrutusGrid;///(chunksX, chunksY, chunksZ);
	auto total = grid.total_size();
	result.Density.Init(1.f, total.y * total.z);
	result.Position.Init(FVector(0.f), total.y * total.z);

	for (int32 z = 0; z < (int32)total.z; z++)
	{
		for (int32 y = 0; y < (int32)total.y; y++)
		{
			int index = z * total.y + y;
			result.Density[index] = grid(InX, y, z).weight;
			result.Position[index] = this->GetActorLocation() + FVector(InX * InChunkManagerInformation.VoxelSize, 
																		y * InChunkManagerInformation.VoxelSize,
																		z * InChunkManagerInformation.VoxelSize);
		}
	}
	return result;
}
void ACxDynamicLandScape::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ACxDynamicLandScape::ChunkExec()
{
	const float ChunkSize = ((BRUTUS_CHUNK_SIZE * this->LandscapeDimension.X ) - 1) * this->VoxelSize;
	FVector PlayerLocation2VoxelLocation = PlayerLocation / ChunkSize;
	PlayerLocation2VoxelLocation.X = FMath::FloorToInt32(PlayerLocation2VoxelLocation.X) * ChunkSize;
	PlayerLocation2VoxelLocation.Y = FMath::FloorToInt32(PlayerLocation2VoxelLocation.Y) * ChunkSize;
	PlayerLocation2VoxelLocation.Z = FMath::FloorToInt32(PlayerLocation2VoxelLocation.Z) * ChunkSize;

	TArray<FChunkManagerInformation> LOD0;
	TArray<FChunkManagerInformation> LOD1;

	int32 s = 8;
	for (int32 w = -s; w < s; w++)
	{
		for (int32 h = -s; h < s; h++)
		{

			int32 x = w * ChunkSize;
			int32 y = h * ChunkSize;
			auto KEY = FVector(x, y, 0) + FVector(PlayerLocation2VoxelLocation.X, PlayerLocation2VoxelLocation.Y, 0);
			auto ChunkPointer = ChunkManager.Find(KEY);
			auto ChunkDistSquared = FVector2D::DistSquared(FVector2D(this->PlayerLocation), FVector2D(KEY));

			if (!ChunkPointer && ChunkDistSquared < LoadedRangeSqared)
			{
				// 没有找到Chunk，需要重新创建。

				float Alpha = ChunkDistSquared / this->LoadedRangeSqared;
				FChunkManagerInformation ChunkManagerInformation;


				if (Alpha < 0.3f)
				{
					// LOD0
					ChunkManagerInformation.LandscapeDimension = this->LandscapeDimension;
					ChunkManagerInformation.VoxelSize = this->VoxelSize;
					ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X,
																		  ChunkManagerInformation.LandscapeDimension.Y,
																		  ChunkManagerInformation.LandscapeDimension.Z);
					ChunkManagerInformation.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
					ChunkManagerInformation.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
					ChunkManagerInformation.MeshComponent->RegisterComponent();
					ChunkManagerInformation.MeshComponent->SetWorldLocation(FVector(KEY.X, KEY.Y, 0));
					ChunkManagerInformation.MeshComponent->DynamicLandScape = this;
					ChunkManagerInformation.MeshComponent->Location = KEY;
					ChunkManagerInformation.MeshComponent->LOD = 0;
					LOD0.Add(ChunkManagerInformation);
				}
				else
				{
					// LOD1
					ChunkManagerInformation.LandscapeDimension = (this->LandscapeDimension / 2) + FVector(1, 1, 0);
					ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X, 
																		  ChunkManagerInformation.LandscapeDimension.Y, 
																		  ChunkManagerInformation.LandscapeDimension.Z);
					ChunkManagerInformation.VoxelSize = this->VoxelSize * 2;
					ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 4;
					ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 4;

					ChunkManagerInformation.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
					ChunkManagerInformation.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
					ChunkManagerInformation.MeshComponent->RegisterComponent();
					ChunkManagerInformation.MeshComponent->SetWorldLocation(FVector(KEY.X, KEY.Y, 0));
					ChunkManagerInformation.MeshComponent->DynamicLandScape = this;
					ChunkManagerInformation.MeshComponent->Location = KEY;
					ChunkManagerInformation.MeshComponent->LOD = 1;
					LOD1.Add(ChunkManagerInformation);
				}
		
				this->ChunkManager.Add(KEY, ChunkManagerInformation);
			}
		}
	}

	for (const auto& it : LOD0)
	{
		CreateDensityField(it);
		UpdateMeshFromDensityMC(it);
	}
	for (const auto& it : LOD1)
	{
		CreateDensityField(it);
		UpdateMeshFromDensityMC(it);
	}
}

void ACxDynamicLandScape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	this->ChunkExec();
}

void ACxDynamicLandScape::CreateTransitionMesh( 
	const Brutus::Size3D InTotal,
	FVector InVector, 
	FVector InLocation,
	float InLength, 
	float InVoxelSize, 
	TArray<FVector>& OutVertices, 
	TArray<int32>& OutTriangle)
{
#ifdef DEBUG_GAMI_CHUNK
	TMap<FIntVector, FString> DebugInformationDrawMap;
#endif

	for (int i = 0; i < InTotal.y; i += 2)
	{
		for (int j = 0; j < InTotal.z; j += 2)
		{
			FVector Pos[13];
			float Density[13] = { 0.f };
			if (InVector.X == 1)
			{


				int32 lx = InTotal.x - 2;

				for (int y = 0; y < 3; y++)
				{
					for (int z = 0; z < 3; z++)
					{
						float Weight = int32(this->GetNoise(lx, i + y , j + z, InTotal, InLocation, InVoxelSize, 0));
						int32 DensityIndex = y * 3 + z;

						Density[DensityIndex] = Weight;
						Pos[DensityIndex] = FVector(lx, y + i, z + j) * InVoxelSize + InLocation;
						Pos[DensityIndex] = Pos[DensityIndex];	/** < 向内部偏移三个像素点 */
#ifdef DEBUG_GAMI_CHUNK
						DrawDebugSphere(this->GetWorld(), 
							Pos[DensityIndex] + InVector * InLength, 2.f, 1.f, FColor::Red, true);
						DebugInformationDrawMap.Add(FIntVector(Pos[DensityIndex] + InVector * InLength),
							FString::Printf(TEXT("%f"), Weight));
#endif
					}
				}

				Pos[9] = Pos[0] + (InVector * InLength);
				Pos[10] = Pos[2] + (InVector * InLength);
				Pos[11] = Pos[6] + (InVector * InLength);
				Pos[12] = Pos[8] + (InVector * InLength);
				Density[9]  = int8(GetNoise(Pos[9], InTotal, InLocation, InVoxelSize, 1));
				Density[10] = int8(GetNoise(Pos[10], InTotal, InLocation, InVoxelSize, 1));
				Density[11] = int8(GetNoise(Pos[11], InTotal, InLocation, InVoxelSize, 1));
				Density[12] = int8(GetNoise(Pos[12], InTotal, InLocation, InVoxelSize, 1));

				DrawDebugString(this->GetWorld(), Pos[9] - InVector * InVoxelSize, FString::Printf(TEXT("%s|%f"), *Pos[9].ToString(), Density[9]), 0, FColor::Black);
				DrawDebugString(this->GetWorld(), Pos[10]- InVector * InVoxelSize, FString::Printf(TEXT("%s|%f"), *Pos[10].ToString(), Density[10]), 0, FColor::Black);
				DrawDebugString(this->GetWorld(), Pos[11]- InVector * InVoxelSize, FString::Printf(TEXT("%s|%f"), *Pos[11].ToString(), Density[11]), 0, FColor::Black);
				DrawDebugString(this->GetWorld(), Pos[12]- InVector * InVoxelSize, FString::Printf(TEXT("%s|%f"), *Pos[12].ToString(), Density[12]), 0, FColor::Black);

			
			}
			else if (InVector.X == -1)
			{

			}
			else if (InVector.Y == 1)
			{
	
			}
			else if (InVector.Y == -1)
			{
	
			}
			else if (InVector.Z == 1)
			{
	
			}
			else if (InVector.Z == -1)
			{
	
			}


			uint16_t caseIndex = 0;
			static const uint16 TransitionCaseBit[9] =
			{
				0x001, // sample 0
				0x002, // sample 1
				0x004, // sample 2
				0x080, // sample 3
				0x100, // sample 4
				0x008, // sample 5
				0x040, // sample 6
				0x020, // sample 7
				0x010  // sample 8
			};

			for (int k = 0; k < 9; ++k)
			{
				if (Density[k] < 0.0f)
					caseIndex |= TransitionCaseBit[k];
			}
			if (caseIndex == 0 || caseIndex == 0x1FF)
				continue;
			const unsigned char rawClass = transitionCellClass[caseIndex];
			bool bFlip = (rawClass & 0x80) != 0;
			const int32 cellClass = rawClass & 0x7F;

			const TransitionCellData& cellData = transitionCellData[cellClass];
			const int32 VertexCount = cellData.GetVertexCount();
			const int32 TriangleCount = cellData.GetTriangleCount();

			TArray<FVector> cellVerts;
			cellVerts.SetNum(VertexCount);

			for (int k = 0; k < VertexCount; ++k)
			{
				const unsigned short ed = transitionVertexData[caseIndex][k];
				const unsigned char ep = ed & 0xFF;
				const int32 a = ep & 0x0F;
				const int32 b = ep >> 4;

				check(a >= 0 && a < 13 && b >= 0 && b < 13);

				float t = 0.5f;
				if (Density[a] != Density[b])
					t = (0.0f - Density[a]) / (Density[b] - Density[a]);

				cellVerts[k] = FMath::Lerp(Pos[a], Pos[b], t);
			}

			const int32 base = OutVertices.Num();
			for (int k = 0; k < VertexCount; ++k)
				OutVertices.Add(cellVerts[k]);

			const unsigned char* triIndices = cellData.vertexIndex;


			if (InVector.X == 1)
			{
				bFlip = !bFlip;
			}

			if (bFlip)
			{

				for (int t = 0; t < TriangleCount * 3; ++t)
					OutTriangle.Add(base + triIndices[t]);
			}
			else
			{
				// 翻转绕序：0,1,2 → 0,2,1
				for (int t = 0; t < TriangleCount; ++t)
				{
					OutTriangle.Add(base + triIndices[t * 3 + 0]);
					OutTriangle.Add(base + triIndices[t * 3 + 2]);
					OutTriangle.Add(base + triIndices[t * 3 + 1]);
				}
			}
		}
	}


#ifdef DEBUG_GAMI_CHUNK
	for (const auto& it : DebugInformationDrawMap)
	{
		const FIntVector& Key = it.Key;     // FIntVector
		const FString& Value = it.Value;   // FString
		DrawDebugString(this->GetWorld(), FVector(Key), Value, 0, FColor::White);
	} 
#endif

}

void ACxDynamicLandScape::CreateDensityField(const FChunkManagerInformation& InChunkManagerInformation)
{
	Brutus::Grid& grid = *InChunkManagerInformation.BrutusGrid;
	Brutus::Size3D total = grid.total_size();  // 64, 64, 64
	const FVector worldOffset = InChunkManagerInformation.MeshComponent->Location;
	for (int32 z = 0; z < (int32)total.z ; ++z)
	{
		for (int32 y = 0; y < (int32)total.y ; ++y)
		{
			for (int32 x = 0; x < (int32)total.x ; ++x)
			{
				float Weight = this->GetNoise(x, y, z, total, worldOffset, InChunkManagerInformation.VoxelSize, InChunkManagerInformation.MeshComponent->LOD);
#ifdef USE_FLAT
				grid(x, y, z).weight = (z * InChunkManagerInformation.VoxelSize - 1000) < 0 ? -1 : 1;
#else
				grid(x, y, z).weight = Weight;
#endif

				if (x == 0)
				{
					auto Position = FVector(x, y, z)* FVector(InChunkManagerInformation.VoxelSize) + InChunkManagerInformation.MeshComponent->Location;

					DrawDebugString(this->GetWorld(),
						Position,
						FString::Printf(TEXT("%s|%f"), *Position.ToString(), Weight), 0, FColor::Green);
				}
			}
		}
	}
}

void ACxDynamicLandScape::UpdateMeshFromDensityMC(const FChunkManagerInformation& InChunkManagerInformation)
{

	const float ChunkSize = (BRUTUS_CHUNK_SIZE * (InChunkManagerInformation.LandscapeDimension.X - 1)) * InChunkManagerInformation.VoxelSize;
	const float LOD1 = BRUTUS_CHUNK_SIZE * (InChunkManagerInformation.LandscapeDimension.X - 1) - 1;

	const int32 chunksX = InChunkManagerInformation.LandscapeDimension.X;
	const int32 chunksY = InChunkManagerInformation.LandscapeDimension.Y;
	const int32 chunksZ = InChunkManagerInformation.LandscapeDimension.Z;
	Brutus::Grid& grid = *InChunkManagerInformation.BrutusGrid;

	TArray<FVector> OutVerts;
	TArray<int32> OutTris;
	TArray<FVector> OutNormals;
	TArray<FVector2D> uvs;
	TArray<FColor> vertexColors;
	TArray<FProcMeshTangent> tangents;
	int32 vertexOffset = 0;
	float XVoxelOffset = 0.f;
	float XVoxelScale = 1.f;
	float YVoxelOffset = 0.f;
	float YVoxelScale = 1.f;

	for (int32 cz = 0; cz < chunksZ; ++cz)
	{
		for (int32 cy = 0; cy < chunksY; ++cy)
		{
			for (int32 cx = 0; cx < chunksX; ++cx)
			{
				// 生成这个 chunk 的网格
				Brutus::Mesh mesh = grid.generate_mesh(cx, cy, cz);

				if (mesh.vertex_count == 0) 
					continue;

		
				for (size_t i = 0; i < mesh.vertex_count; ++i)
				{

					XVoxelOffset = 0.f;
					XVoxelScale = 1.f;
					YVoxelOffset = 0.f;
					YVoxelScale = 1.f;

					FVector Vertices(
						mesh.vertices[i * 3],
						mesh.vertices[i * 3 + 1],
						mesh.vertices[i * 3 + 2]
					);

					if (InChunkManagerInformation.MeshComponent->LOD == 1)
					{
						auto* it = grid.GetMapInstance().Find(i * 3);
						if (it)
						{
							if (it->x == LOD1)
							{
								XVoxelOffset = (ChunkSize / 2) - this->VoxelSize;
								XVoxelScale = 0.5;
							}
							if (it->y == LOD1)
							{
								YVoxelOffset = (ChunkSize / 2) - this->VoxelSize;
								YVoxelScale = 0.5;
							}
						}
					}

					FVector WorldVertices = Vertices * FVector(
						InChunkManagerInformation.VoxelSize,
						InChunkManagerInformation.VoxelSize,
						InChunkManagerInformation.VoxelSize)
						*
						FVector(XVoxelScale, YVoxelScale, 1.f)
						+
						FVector(XVoxelOffset, YVoxelOffset, 0);
					OutVerts.Add(WorldVertices);
				}

				for (size_t i = 0; i < mesh.vertex_count; i += 3)
				{
					OutTris.Add(vertexOffset + static_cast<int32>(i + 0));
					OutTris.Add(vertexOffset + static_cast<int32>(i + 2)); // 交换 1 和 2
					OutTris.Add(vertexOffset + static_cast<int32>(i + 1));
				}

				vertexOffset += static_cast<int32>(mesh.vertex_count);
			}
		}
	}

	///grid.MeshEnd();
	if (OutVerts.Num() > 0)
	{
		InChunkManagerInformation.MeshComponent->CreateMeshSection(0, OutVerts, OutTris, OutNormals, uvs, vertexColors, tangents, true);
		if (VoxelMaterial)
		{
			InChunkManagerInformation.MeshComponent->SetMaterial(0, VoxelMaterial);
		}
	}
}

void ACxDynamicLandScape::UpdateLOD(FVector InKEY, float InDistSqared)
{

	auto it = this->ChunkManager.Find(InKEY);
	if (!it)
		return;
	auto& ChunkManagerInformation = *it;
	auto Alpha = InDistSqared / this->LoadedRangeSqared;
	bool bUpadate = false;
	if (Alpha < 0.3f && ChunkManagerInformation.MeshComponent->LOD != 0)
	{
		delete ChunkManagerInformation.BrutusGrid;
		// LOD0
		ChunkManagerInformation.LandscapeDimension = this->LandscapeDimension;
		ChunkManagerInformation.VoxelSize = this->VoxelSize;
		ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X,
															  ChunkManagerInformation.LandscapeDimension.Y,
															  ChunkManagerInformation.LandscapeDimension.Z);
		ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 1;
		ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 1;
		ChunkManagerInformation.MeshComponent->LOD = 0;
		bUpadate = true;
	}
	else if(Alpha >= 0.3f && ChunkManagerInformation.MeshComponent->LOD != 1)
	{
		// LOD1
		delete ChunkManagerInformation.BrutusGrid;

		ChunkManagerInformation.LandscapeDimension = (this->LandscapeDimension / 2) + FVector(1, 1, 0);
		ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X,
															  ChunkManagerInformation.LandscapeDimension.Y,
															  ChunkManagerInformation.LandscapeDimension.Z);
		ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 4;
		ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 4;
		ChunkManagerInformation.VoxelSize = this->VoxelSize * 2;
		ChunkManagerInformation.MeshComponent->LOD = 1;
		bUpadate = true;
	}
	if (bUpadate)
	{
		CreateDensityField(*it);
		UpdateMeshFromDensityMC(*it);
	}
}

void ACxDynamicLandScape::SetPlayerLocation(FVector InLocation)
{
	PlayerLocation = InLocation;
}

float ACxDynamicLandScape::GetNoise(float InX, float InY, float InZ, const Brutus::Size3D& InTotal,const FVector &InComponentLocation, float InVoxelSize, uint8 InLOD)
{
	const float DensityMin = -BaseHeight - NoiseHeight;
	const float DensityMax = ((InTotal.z - 1) * InVoxelSize) - BaseHeight + NoiseHeight; 

	float sampleX = InX * InVoxelSize + InComponentLocation.X;
	float sampleY = InY * InVoxelSize + InComponentLocation.Y;
	float sampleZ = InZ * InVoxelSize + InComponentLocation.Z;

	if (InLOD != 0)
	{
		if (InX == (InTotal.x - 4))
		{
			int32 start = (InX - 1) * InVoxelSize + InComponentLocation.X;
			sampleX = start + InVoxelSize / 2.f;
		}
		if (InY == (InTotal.y - 4))
		{
			int32 start = (InY - 1) * InVoxelSize + InComponentLocation.Y;
			sampleY = start + InVoxelSize / 2.f;
		}
	}

	sampleX *= NoiseFreq;
	sampleY *= NoiseFreq;
	sampleZ *= NoiseFreq;
	float noiseValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * NoiseHeight;
	float Density = (InZ * InVoxelSize) - (BaseHeight + noiseValue);
	float Weight = FMath::GetMappedRangeValueClamped(
		FVector2D(DensityMin, DensityMax),
		FVector2D(-127.0f, 127.0f),
		Density
	);

	return Weight;
}

inline float ACxDynamicLandScape::GetNoise(FVector InLocation,
	const Brutus::Size3D& InTotal,
	const FVector& InComponentLocation, 
	float InVoxelSize, 
	uint8 InLOD)
{
	const float DensityMin = -BaseHeight - NoiseHeight;
	const float DensityMax = ((InTotal.z - 1) * InVoxelSize) - BaseHeight + NoiseHeight;

	float sampleX = InLocation.X;
	float sampleY = InLocation.Y;
	float sampleZ = InLocation.Z;

	sampleX *= NoiseFreq;
	sampleY *= NoiseFreq;
	sampleZ *= NoiseFreq;

	float noiseValue = FMath::PerlinNoise2D(FVector2D(sampleX, sampleY)) * NoiseHeight;
	float Density = InLocation.Z - (BaseHeight + noiseValue);
	float Weight = FMath::GetMappedRangeValueClamped(
		FVector2D(DensityMin, DensityMax),
		FVector2D(-127.0f, 127.0f),
		Density
	);

	return Weight;
}

bool ACxDynamicLandScape::CircleChunk(FVector InLocation)
{
	auto it = this->ChunkManager.Find(InLocation);
	if (it)
	{
		delete it->BrutusGrid;
		it->MeshComponent->DestroyComponent();
		this->ChunkManager.Remove(InLocation);
		return true;
	}

	return false;
	
}