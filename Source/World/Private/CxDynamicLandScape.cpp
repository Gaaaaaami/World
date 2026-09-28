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


#if 1
#define DEBUG_GAMI_CHUNK
#endif

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
	TArray<FVector> POS = {
		FVector(1400,1300,700),
		FVector(1400,1400,700),
		FVector(1400,1300,800),
		FVector(1400,1400,800)
	};
	TArray<float> Density = {
		-13,
		-12,
		0,
		0
	};
	TArray<FVector> PosRes;
	TArray<float> DensityRes;
	this->ConvertSquareToNineGrid(Density, POS, DensityRes, PosRes);
	int index = 0;
	for (auto it : PosRes)
	{
		DrawDebugSphere(this->GetWorld(), it, 2.f, 3, FColor::Red, true, -1.f);
		DrawDebugString(this->GetWorld(), it, FString::Printf(TEXT("%s, %f"), *it.ToString(), DensityRes[index]), 0, FColor::White, -1.f, true);
		index++;
	}
	// 假设小矩形位于大矩形内部，例如中心区域
	TArray<FVector> InnerPos = {
		//FVector(1500, 1200, 600),
		//FVector(1500, 1400, 600),
		//FVector(1500, 1200, 800),
		//FVector(1500, 1400, 800),
		FVector(1500, 1300, 700),
		FVector(1500, 1400, 700),
		FVector(1500, 1300, 800),
		FVector(1500, 1400, 800)
	};
	TArray<float> DensityRes1;
	TArray<FVector> POS0 = {
		FVector(1500, 1200, 600), 
		FVector(1500, 1400, 600),  
		FVector(1500, 1200, 800), 
		FVector(1500, 1400, 800), 
	};

	TArray<float> Density0 = {
		-27,  // (0,1)
		-21,   // (1,1)
		0,    // (0,0)
		5,    // (1,0)
	};

	GetInnerRectDensity(POS0, Density0, InnerPos, DensityRes1);
	index = 0;
	for (auto it : InnerPos)
	{
		//DrawDebugSphere(this->GetWorld(), it, 2.f, 3, FColor::Red, true, -1.f);
		//DrawDebugString(this->GetWorld(), it, FString::Printf(TEXT("%s, %f"), *it.ToString(), DensityRes1[index]), 0, FColor::Green, -1.f, true);
		index++;
	}




	FVector TransvoxelPos[13];
	float TransDensity[13] = { 0.f };

	index = 0;
	for (auto it : PosRes)
	{
		TransvoxelPos[index] = it;
		TransDensity[index] = DensityRes[index];
		index++;
	}
	TransvoxelPos[9] = InnerPos[0] + FVector(1,0,0);
	TransvoxelPos[10] = InnerPos[2]+ FVector(1,0,0);
	TransvoxelPos[11] = InnerPos[1]+ FVector(1,0,0);
	TransvoxelPos[12] = InnerPos[3]+ FVector(1,0,0);
	TransDensity[9] = DensityRes1[0];
	TransDensity[10] = DensityRes1[2];
	TransDensity[11] = DensityRes1[1];
	TransDensity[12] = DensityRes1[3];

#if 1
	FChunkManagerInformation ChunkManagerInformation;
	ChunkManagerInformation.LandscapeDimension = this->LandscapeDimension;
	ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X,
		ChunkManagerInformation.LandscapeDimension.Y,
		ChunkManagerInformation.LandscapeDimension.Z);
	ChunkManagerInformation.VoxelSize = this->VoxelSize;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 2;
	ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 2;

	ChunkManagerInformation.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation.MeshComponent->RegisterComponent();
	ChunkManagerInformation.MeshComponent->SetWorldLocation(FVector(0, 0, 0));
	ChunkManagerInformation.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation.MeshComponent->Location = FVector(0);
	ChunkManagerInformation.MeshComponent->LOD = 0;
	ChunkManagerInformation.DebugColor = FColor::Green;
	//LOD1.Add(ChunkManagerInformation);
	CreateDensityField(ChunkManagerInformation);
	UpdateMeshFromDensityMC(ChunkManagerInformation);

	Brutus::Size3D total = { this->LandscapeDimension.X * BRUTUS_CHUNK_SIZE,
							 this->LandscapeDimension.Y * BRUTUS_CHUNK_SIZE,
							 this->LandscapeDimension.Z * BRUTUS_CHUNK_SIZE };
	auto DensityInformation =  GetDensityX(total.x - 1, ChunkManagerInformation);
	
	TArray<FVector> TransitionVertices;
	TArray<int32> TransitionTriangle;
	TArray<float> LowRes;
	TArray<FVector> LowPos;

	//ADebugTransvoxel* DebugTransvoxel = NewObject<ADebugTransvoxel>(this);
	//DebugTransvoxel->BuildTransition_MultiUnit(TransitionVertices, TransitionTriangle, DensityInformation.Density,
	//	LowRes, DensityInformation.Position, LowPos, FVector(15, 15, 15),
	//	FVector(7, 7, 7), FVector(1, 0, 0), 100.f, ChunkManagerInformation.VoxelSize);

#if 1

	CreateTransitionMesh(
		total,
		FVector(1, 0, 0),
		FVector(0, 0, 0),
		100.f,
		100.f,
		TransitionVertices,
		TransitionTriangle
	);
#endif
	TArray<FVector> TransitionVertices0;
	TArray<int32> TransitionTriangle0;
	this->CreateTransitionMesh(TransvoxelPos, TransDensity, FVector(1, 0, 0), TransitionVertices0, TransitionTriangle0);



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
	ChunkManagerInformation.MeshComponent->CreateMeshSection(2,
		TransitionVertices0,
		TransitionTriangle0,
		Normals,
		UVS,
		Color,
		Tangents,
		false);
	UE_LOG(LogTemp, Log, TEXT("Vertices Nums : %d, Triangles Nums : %d"), TransitionVertices.Num(), TransitionTriangle.Num());

	FChunkManagerInformation ChunkManagerInformation0;
	ChunkManagerInformation0.LandscapeDimension = this->LandscapeDimension;
	ChunkManagerInformation0.BrutusGrid = new Brutus::Grid(ChunkManagerInformation0.LandscapeDimension.X,
		ChunkManagerInformation0.LandscapeDimension.Y,
		ChunkManagerInformation0.LandscapeDimension.Z);
	ChunkManagerInformation0.VoxelSize = this->VoxelSize;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation0.BrutusGrid->XEndDeleteVoxel = 2;
	ChunkManagerInformation0.BrutusGrid->YEndDeleteVoxel = 2;

	ChunkManagerInformation0.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation0.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation0.MeshComponent->RegisterComponent();
	ChunkManagerInformation0.MeshComponent->SetWorldLocation(FVector(0, 1500, 0));
	ChunkManagerInformation0.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation0.MeshComponent->Location = FVector(0, 1500, 0);

	ChunkManagerInformation0.MeshComponent->LOD = 0;
	//LOD1.Add(ChunkManagerInformation);

	//CreateDensityField(ChunkManagerInformation0);
	//UpdateMeshFromDensityMC(ChunkManagerInformation0);

	FChunkManagerInformation ChunkManagerInformation1;
	ChunkManagerInformation1.LandscapeDimension = (this->LandscapeDimension / 2);
	ChunkManagerInformation1.BrutusGrid = new Brutus::Grid(ChunkManagerInformation1.LandscapeDimension.X,
		ChunkManagerInformation1.LandscapeDimension.Y,
		ChunkManagerInformation1.LandscapeDimension.Z);
	ChunkManagerInformation1.VoxelSize = this->VoxelSize * 2;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation1.BrutusGrid->XEndDeleteVoxel = 1;
	ChunkManagerInformation1.BrutusGrid->YEndDeleteVoxel = 1;

	ChunkManagerInformation1.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation1.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation1.MeshComponent->RegisterComponent();
	ChunkManagerInformation1.MeshComponent->SetWorldLocation(FVector(1500, 0, 0));
	ChunkManagerInformation1.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation1.MeshComponent->Location = FVector(1500, 0, 0);
	ChunkManagerInformation1.MeshComponent->LOD = 1;
	ChunkManagerInformation1.DebugColor = FColor::Red;
	CreateDensityField(ChunkManagerInformation1);
	UpdateMeshFromDensityMC(ChunkManagerInformation1);


	//FVector Noise0Location(0);
	//float Noise0 = GetNoise(15, 0, 0, total, Noise0Location, 100.f, 0);
	//float Noise1 = GetNoise(FVector(0, 0, 0), total, ChunkManagerInformation0.MeshComponent->Location, 100.f, 1);

#elif 0
	FChunkManagerInformation ChunkManagerInformation;
	ChunkManagerInformation.LandscapeDimension = this->LandscapeDimension;
	ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X,
		ChunkManagerInformation.LandscapeDimension.Y,
		ChunkManagerInformation.LandscapeDimension.Z);
	ChunkManagerInformation.VoxelSize = this->VoxelSize;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 2;
	ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 2;

	ChunkManagerInformation.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation.MeshComponent->RegisterComponent();
	ChunkManagerInformation.MeshComponent->SetWorldLocation(FVector(0, 0, 0));
	ChunkManagerInformation.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation.MeshComponent->Location = FVector(0);
	ChunkManagerInformation.MeshComponent->LOD = 0;

	ChunkManagerInformation.DebugColor = FColor::Red;

	//LOD1.Add(ChunkManagerInformation);
	CreateDensityField(ChunkManagerInformation);
	UpdateMeshFromDensityMC(ChunkManagerInformation);

	Brutus::Size3D total = { this->LandscapeDimension.X * BRUTUS_CHUNK_SIZE,
							 this->LandscapeDimension.Y * BRUTUS_CHUNK_SIZE,
							 this->LandscapeDimension.Z * BRUTUS_CHUNK_SIZE };


	FChunkManagerInformation ChunkManagerInformation1;
	ChunkManagerInformation1.LandscapeDimension = (this->LandscapeDimension / 2);
	ChunkManagerInformation1.BrutusGrid = new Brutus::Grid(ChunkManagerInformation1.LandscapeDimension.X,
		ChunkManagerInformation1.LandscapeDimension.Y,
		ChunkManagerInformation1.LandscapeDimension.Z);
	ChunkManagerInformation1.VoxelSize = this->VoxelSize * 2;
	//ChunkManagerInformation.Location = KEY;
	ChunkManagerInformation1.BrutusGrid->XEndDeleteVoxel = 1;
	ChunkManagerInformation1.BrutusGrid->YEndDeleteVoxel = 1;

	ChunkManagerInformation1.MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
	ChunkManagerInformation1.MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	ChunkManagerInformation1.MeshComponent->RegisterComponent();
	ChunkManagerInformation1.MeshComponent->SetWorldLocation(FVector(-1400, -1400, 0));
	ChunkManagerInformation1.MeshComponent->DynamicLandScape = this;
	ChunkManagerInformation1.MeshComponent->Location = FVector(-1400, -1400, 0);
	ChunkManagerInformation1.MeshComponent->LOD = 1;
	ChunkManagerInformation1.DebugColor = FColor::Blue;
	ChunkManagerInformation1.BrutusGrid->SkipVoxelLocation.Add(FIntVector(0, 0, -1));
	ChunkManagerInformation1.BrutusGrid->SkipVoxelLocation.Add(FIntVector(1, 0, -1));
	ChunkManagerInformation1.BrutusGrid->SkipVoxelLocation.Add(FIntVector(2, 0, -1));
	ChunkManagerInformation1.BrutusGrid->SkipVoxelLocation.Add(FIntVector(1, 1, -1));

	CreateDensityField(ChunkManagerInformation1);
	UpdateMeshFromDensityMC(ChunkManagerInformation1);

#endif
	///this->ChunkExec();
}

void ACxDynamicLandScape::GetInnerRectDensity(
	const TArray<FVector>& InPos,
	const TArray<float>& InDensity,
	const TArray<FVector>& InnerPos,
	TArray<float>& OutInnerDensity)
{
	OutInnerDensity.Reset();

	// 输入校验
	if (InPos.Num() < 4 || InDensity.Num() < 4 || InnerPos.Num() < 4)
	{
		return;
	}

	// 大矩形四个角点，顺序 (0,0)(1,0)(0,1)(1,1)
	const FVector& P00 = InPos[0];
	const FVector& P10 = InPos[1];
	const FVector& P01 = InPos[2];
	const FVector& P11 = InPos[3];

	const float D00 = InDensity[0];
	const float D10 = InDensity[1];
	const float D01 = InDensity[2];
	const float D11 = InDensity[3];

	// 大矩形在 Y 和 Z 方向的范围
	const double Y0 = P00.Y;
	const double Y1 = P10.Y;
	const double Z0 = P00.Z;
	const double Z1 = P01.Z;

	// 避免除零
	check(Y1 != Y0);
	check(Z1 != Z0);

	OutInnerDensity.Reserve(4);

	// 处理小矩形四个角点
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector& P = InnerPos[i];

		// 计算归一化坐标
		const double u = (P.Y - Y0) / (Y1 - Y0);
		const double v = (P.Z - Z0) / (Z1 - Z0);

		// 必须在大矩形内部
		check(!(u < 0.0 || u > 1.0 || v < 0.0 || v > 1.0));

		// 双线性插值密度
		const double w00 = (1.0 - u) * (1.0 - v);
		const double w10 = u * (1.0 - v);
		const double w01 = (1.0 - u) * v;
		const double w11 = u * v;

		const float OutD = static_cast<float>(
			D00 * w00 + D10 * w10 + D01 * w01 + D11 * w11);

		OutInnerDensity.Add(OutD);
	}
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
	const float ChunkSize = ((BRUTUS_CHUNK_SIZE * this->LandscapeDimension.X ) - 2) * this->VoxelSize;
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
					ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 2;
					ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 2;

					ChunkManagerInformation.MeshComponent->SetWorldLocation(FVector(KEY.X, KEY.Y, 0));
					ChunkManagerInformation.MeshComponent->DynamicLandScape = this;
					ChunkManagerInformation.MeshComponent->Location = KEY;
					ChunkManagerInformation.MeshComponent->LOD = 0;
					LOD0.Add(ChunkManagerInformation);
				}
				else
				{
					// LOD1
					ChunkManagerInformation.LandscapeDimension = (this->LandscapeDimension / 2);
					ChunkManagerInformation.BrutusGrid = new Brutus::Grid(ChunkManagerInformation.LandscapeDimension.X, 
																		  ChunkManagerInformation.LandscapeDimension.Y, 
																		  ChunkManagerInformation.LandscapeDimension.Z);
					ChunkManagerInformation.VoxelSize = this->VoxelSize * 2;
					ChunkManagerInformation.BrutusGrid->XEndDeleteVoxel = 1;
					ChunkManagerInformation.BrutusGrid->YEndDeleteVoxel = 1;

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
		const auto &Location = it.MeshComponent->Location;
		//循环遍历周围九宫格，看看有没有比自己低一个单位的LOD
		bool x1 = TransitionMeshCheck(Location + FVector(ChunkSize, 0, 0), 0);	 // 检查方向 FVector(1, 0, 0) 的位置是否存在Chunk
		bool xs1 = TransitionMeshCheck(Location + FVector(-ChunkSize, 0, 0), 0);	 // 检查方向 FVector(-1, 0, 0) 的位置是否存在Chunk
		bool y1 = TransitionMeshCheck(Location + FVector(0, ChunkSize, 0), 0);	 // 检查方向 FVector(1, 0, 0) 的位置是否存在Chunk
		bool ys1 = TransitionMeshCheck(Location + FVector(0, -ChunkSize, 0), 0);	 // 检查方向 FVector(-1, 0, 0) 的位置是否存在Chunk

#if 0
		// 开始对Z轴进行修改的时候才开始，动态加载Z轴
		bool x2 = TransitionMeshCheck(Location + FVector(ChunkSize, 0, 0), 0);	 // 检查方向 FVector(1, 0, 0) 的位置是否存在Chunk
		bool xs2 = TransitionMeshCheck(Location + FVector(-ChunkSize, 0, 0), 0);	 // 检查方向 FVector(-1, 0, 0) 的位置是否存在Chunk
		bool y2 = TransitionMeshCheck(Location + FVector(0, ChunkSize, 0), 0);	 // 检查方向 FVector(1, 0, 0) 的位置是否存在Chunk
		bool ys2 = TransitionMeshCheck(Location + FVector(0, -ChunkSize, 0), 0);	 // 检查方向 FVector(-1, 0, 0) 的位置是否存在Chunk

		bool x3 = TransitionMeshCheck(Location + FVector(ChunkSize, 0, 0), 0);	 // 检查方向 FVector(1, 0, 0) 的位置是否存在Chunk
		bool xs3 = TransitionMeshCheck(Location + FVector(-ChunkSize, 0, 0), 0);	 // 检查方向 FVector(-1, 0, 0) 的位置是否存在Chunk
		bool y3 = TransitionMeshCheck(Location + FVector(0, ChunkSize, 0), 0);	 // 检查方向 FVector(1, 0, 0) 的位置是否存在Chunk
		bool ys3 = TransitionMeshCheck(Location + FVector(0, -ChunkSize, 0), 0);	 // 检查方向 FVector(-1, 0, 0) 的位置是否存在Chunk
#endif

		if (x1)
		{
			it.BrutusGrid->XEndDeleteVoxel = 3;
		}
		if (xs1)
		{
			it.BrutusGrid->XBeginDeleteVoxel = 1;
		}
		if (y1)
		{
			it.BrutusGrid->YEndDeleteVoxel = 3;
		}
		if (ys1)
		{
			it.BrutusGrid->YBeginDeleteVoxel = 1;
		}

		if (!x1 && !xs1 && !y1 && !ys1)
		{

			bool kx1 = TransitionMeshCheck(Location + FVector(ChunkSize, ChunkSize, 0), 0 );	 // 检查方向 FVector(1, 1, 0) 的位置是否存在Chunk
			bool kxs1 = TransitionMeshCheck(Location + FVector(ChunkSize, -ChunkSize, 0), 0);	 // 检查方向 FVector(1, -1, 0) 的位置是否存在Chunk
			bool ky1 = TransitionMeshCheck(Location + FVector(-ChunkSize, ChunkSize, 0), 0);	 // 检查方向 FVector(-1, 1, 0) 的位置是否存在Chunk
			bool kys1 = TransitionMeshCheck(Location + FVector(-ChunkSize, -ChunkSize, 0), 0);	 // 检查方向 FVector(-1, -1, 0) 的位置是否存在Chunk
			const int32 x = it.LandscapeDimension.X * BRUTUS_CHUNK_SIZE - it.BrutusGrid->XEndDeleteVoxel - 1;
			const int32 y = it.LandscapeDimension.Y * BRUTUS_CHUNK_SIZE - it.BrutusGrid->YEndDeleteVoxel - 1;
			if (kx1 )
			{
				it.BrutusGrid->SkipVoxelLocation.Add(FIntVector(x, y, -1));
			}
			if (kxs1)
			{
				it.BrutusGrid->SkipVoxelLocation.Add(FIntVector(x, 0, -1));
			}
			if (ky1)
			{
				it.BrutusGrid->SkipVoxelLocation.Add(FIntVector(0, y, -1));
			}
			if (kys1)
			{
				it.BrutusGrid->SkipVoxelLocation.Add(FIntVector(0, 0, -1));
			}
		}

		CreateDensityField(it);
		UpdateMeshFromDensityMC(it);
	}
	for (const auto& it : LOD1)
	{
		CreateDensityField(it);
		UpdateMeshFromDensityMC(it);
	}
}

void ACxDynamicLandScape::ConvertSquareToNineGrid(
	const TArray<float>& InDensity,
	const TArray<FVector>& InPos,
	TArray<float>& OutDensity,
	TArray<FVector>& OutPos)
{
	// 当前阶段仅实现 OutPos：2x2 -> 3x3 顶点细分
	OutPos.Reset();

	if (InPos.Num() < 4)
	{
		return;
	}

	// 输入顺序约定：
	// InPos[0] -> (u=0, v=0)  行0 列0
	// InPos[1] -> (u=1, v=0)  行1 列0
	// InPos[2] -> (u=0, v=1)  行0 列1
	// InPos[3] -> (u=1, v=1)  行1 列1
	//
	// 其中 u 为行方向，v 为列方向。
	const FVector& P00 = InPos[0]; // 行0 列0
	const FVector& P10 = InPos[1]; // 行1 列0
	const FVector& P01 = InPos[2]; // 行0 列1
	const FVector& P11 = InPos[3]; // 行1 列1

	const float D00 = InDensity[0];
	const float D10 = InDensity[1];
	const float D01 = InDensity[2];
	const float D11 = InDensity[3];

	OutPos.Reserve(9);
	OutDensity.Reserve(9);

	// 输出行优先：外层 u（行），内层 v（列）
	for (int32 Row = 0; Row < 3; ++Row)
	{
		const double u = static_cast<double>(Row) / 2.0; // 0.0, 0.5, 1.0
		for (int32 Col = 0; Col < 3; ++Col)
		{
			const double v = static_cast<double>(Col) / 2.0; // 0.0, 0.5, 1.0

			// 双线性插值
			const double w00 = (1.0 - u) * (1.0 - v);
			const double w10 = u * (1.0 - v);
			const double w01 = (1.0 - u) * v;
			const double w11 = u * v;

			FVector Out = P00 * w00 + P10 * w10 + P01 * w01 + P11 * w11;
			OutPos.Add(Out);


			// 密度插值：边中点退化为线性插值，中心点为四角平均。
			// 保留符号，不做 clamp，服务 Marching Cubes + Transvoxel。
			const float OutD = static_cast<float>(
				D00 * w00 + D10 * w10 + D01 * w01 + D11 * w11);
			OutDensity.Add(OutD);
		}
	}
}
void ACxDynamicLandScape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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

	for (int i = 0; i < InTotal.y - 2; i += 2)
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
#ifdef DEBUG_GAMI_CHUNK
			/*			DrawDebugSphere(this->GetWorld(), 
							Pos[DensityIndex] + InVector * InLength, 2.f, 1.f, FColor::Red, true);
						DebugInformationDrawMap.Add(FIntVector(Pos[DensityIndex] + InVector * InLength),
							FString::Printf(TEXT("%f"), Weight));*/
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


				for (int k = 0; k < 9; k++)
				{
					Pos[k] = Pos[k] + (InVector * -1.f);
				}

				for (int k = 0; k < 4; k++)
				{
					Pos[k+9] = Pos[k+9] + InVector;
				}

#if 0
				DrawDebugString(this->GetWorld(), Pos[9] - InVector * 0.f, FString::Printf(TEXT("%s|%f"), *Pos[9].ToString(), Density[9]), 0, FColor::Black);
				DrawDebugString(this->GetWorld(), Pos[10]- InVector * 0.f, FString::Printf(TEXT("%s|%f"), *Pos[10].ToString(), Density[10]), 0, FColor::Black);
				DrawDebugString(this->GetWorld(), Pos[11]- InVector * 0.f, FString::Printf(TEXT("%s|%f"), *Pos[11].ToString(), Density[11]), 0, FColor::Black);
				DrawDebugString(this->GetWorld(), Pos[12]- InVector * 0.f, FString::Printf(TEXT("%s|%f"), *Pos[12].ToString(), Density[12]), 0, FColor::Black);
#endif
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
	//for (const auto& it : DebugInformationDrawMap)
	//{
	//	const FIntVector& Key = it.Key;     // FIntVector
	//	const FString& Value = it.Value;   // FString
	//	DrawDebugString(this->GetWorld(), FVector(Key), Value, 0, FColor::White);
	//} 
#endif

}

void ACxDynamicLandScape::CreateTransitionMesh(const FVector* InPos, 
	const float* InDensity, 
	FVector InVector,
	TArray<FVector>& OutVertices, 
	TArray<int32>& OutTriangle)
{
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
		if (InDensity[k] < 0.0f)
			caseIndex |= TransitionCaseBit[k];
	}
	if (caseIndex == 0 || caseIndex == 0x1FF)
		return;
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
		if (InDensity[a] != InDensity[b])
			t = (0.0f - InDensity[a]) / (InDensity[b] - InDensity[a]);

		cellVerts[k] = FMath::Lerp(InPos[a], InPos[b], t);
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

#ifdef DEBUG_GAMI_CHUNK
				if ((x == total.x - 2 && InChunkManagerInformation.MeshComponent->LOD == 0) || (x == 0 && InChunkManagerInformation.MeshComponent->LOD == 1))
				{


					if (InChunkManagerInformation.MeshComponent->LOD == 1)
					{
						//Offset.X = 200;
						auto Position = FVector(x, y, z) * FVector(InChunkManagerInformation.VoxelSize) + InChunkManagerInformation.MeshComponent->Location;
						DrawDebugString(this->GetWorld(),
							Position ,
							FString::Printf(TEXT("%s|%f"), *Position.ToString(), float(int8(grid(x, y, z).weight))),
							0, InChunkManagerInformation.DebugColor,-1.f, true, 1.f);
					}



				}
#endif

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

					//if (InChunkManagerInformation.MeshComponent->LOD == 1)
					//{
					//	auto* it = grid.GetMapInstance().Find(i * 3);
					//	if (it)
					//	{
					//		if (it->x == LOD1)
					//		{
					//			XVoxelOffset = (ChunkSize / 2) - this->VoxelSize;
					//			XVoxelScale = 0.5;
					//		}
					//		if (it->y == LOD1)
					//		{
					//			YVoxelOffset = (ChunkSize / 2) - this->VoxelSize;
					//			YVoxelScale = 0.5;
					//		}
					//	}
					//}







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
	float DensityMin = -BaseHeight - NoiseHeight;
	float DensityMax = ((InTotal.z - 1) * InVoxelSize) - BaseHeight + NoiseHeight; 

	float sampleX = InX * InVoxelSize + InComponentLocation.X;
	float sampleY = InY * InVoxelSize + InComponentLocation.Y;
	float sampleZ = InZ * InVoxelSize + InComponentLocation.Z;

	if (InLOD != 0 || InTotal.x == 8)
	{
#if 0
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
#endif
		DensityMax += InVoxelSize / 2;
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

bool ACxDynamicLandScape::TransitionMeshCheck(FVector InKEY, int32 InLOD)
{
	// 用于检查当前坐标点的Chunk是否存在，第二点，检查当前坐标点的LOD是否低于InLODd参数的一个单位
	FChunkManagerInformation* m = this->ChunkManager.Find(InKEY);
	if (m)
	{
		if (m->MeshComponent->LOD == (InLOD + 1))
		{			
			return true;
		}
	}
	return false;
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