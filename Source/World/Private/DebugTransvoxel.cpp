// Fill out your copyright notice in the Description page of Project Settings.


#include "DebugTransvoxel.h"
#include "Transvoxel.h"



void ADebugTransvoxel::BuildTransition_MultiUnit(
	TArray<FVector>& OutVerts,
	TArray<int32>& OutTris,
	const TArray<float>& InHighRes,   
	const TArray<float>& InLowRes,     
	const TArray<FVector>& InHighPos,
	const TArray<FVector>& InLowPos,
	FVector InHighResSize,             
	FVector InLowResSize)             
{

	FVector pos[13];
	float Density[13] = { 1.f };
	float VoxelSize = 100.f;

	int32 Count = InHighResSize.Z / 2;
	Count *= Count;

	LowLODDensitySize = InLowResSize + 1;
	LowLODDensityField.SetNum(LowLODDensitySize.Y * LowLODDensitySize.Z);	/** < 虚幻引擎的Init性能很差，O(n) 级别的耗时 */

	for (int c = 0; c < Count ; c++)
	{
		int32 uy = c % (int32)(InHighResSize.Y / 2); // Y方向第几个单元
		int32 uz = c / (int32)(InHighResSize.Y / 2); // Z方向第几个单元
		
		for (int y = 0; y < 3; y++)
		{
			for (int z = 0; z < 3; z++)
			{
				int HighResindex = (z + (uz * 2)) * (InHighResSize.Y + 1) + y + (uy * 2);
				int DensityIndex = y * 3 + z;
				Density[DensityIndex] = InHighRes[HighResindex];
				pos[DensityIndex] = InHighPos[HighResindex];
			}
		}

		pos[9] =  pos[0] + (FVector(-1, 0, 0) * VoxelSize);
		pos[10] = pos[2] + (FVector(-1, 0, 0) * VoxelSize);
		pos[11] = pos[6] + (FVector(-1, 0, 0) * VoxelSize);
		pos[12] = pos[8] + (FVector(-1, 0, 0) * VoxelSize);
		Density[9] = Density[0];
		Density[10] = Density[2];
		Density[11] = Density[6];
		Density[12] = Density[8];

		int32 n0 = CaculateLowLODDensityFieldIndex(uy, uz);
		int32 n1 = CaculateLowLODDensityFieldIndex(uy + 1, uz);
		int32 n2 = CaculateLowLODDensityFieldIndex(uy, uz + 1);
		int32 n3 = CaculateLowLODDensityFieldIndex(uy + 1, uz + 1);

		LowLODDensityField[n0] = Density[9];
		LowLODDensityField[n1] = Density[11];
		LowLODDensityField[n2] = Density[10];
		LowLODDensityField[n3] = Density[12];

		// ========== 2. 动态计算 caseIndex ==========
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

		for (int i = 0; i < 9; ++i)
		{
			if (Density[i] < 0.0f)
				caseIndex |= TransitionCaseBit[i];
		}

		// 👇 新增：全正/全负直接跳过，不用查表
		if (caseIndex == 0 || caseIndex == 0x1FF)
			continue;

		//UE_LOG(LogTemp, Warning, TEXT("caseIndex = %d"), caseIndex);

		// ========== 3. 查表 ==========
		const unsigned char rawClass = transitionCellClass[caseIndex];
		const bool bFlip = (rawClass & 0x80) != 0;
		const int32 cellClass = rawClass & 0x7F;

		const TransitionCellData& cellData = transitionCellData[cellClass];
		const int32 VertexCount = cellData.GetVertexCount();
		const int32 TriangleCount = cellData.GetTriangleCount();

		// ========== 4. 插值顶点（从表读边） ==========
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

			cellVerts[k] = FMath::Lerp(pos[a], pos[b], t);

		}

		// ========== 5. 输出顶点 ==========
		const int32 base = OutVerts.Num();
		for (int k = 0; k < VertexCount; ++k)
			OutVerts.Add(cellVerts[k]);

		// ========== 6. 输出三角形（从表读索引） ==========
		const unsigned char* triIndices = cellData.vertexIndex; // ← 如果成员名不对请改成你项目里实际的名字

		if (bFlip)
		{
		
			for (int t = 0; t < TriangleCount * 3; ++t)
				OutTris.Add(base + triIndices[t]);
		}
		else
		{
			// 翻转绕序：0,1,2 → 0,2,1
			for (int t = 0; t < TriangleCount; ++t)
			{
				OutTris.Add(base + triIndices[t * 3 + 0]);
				OutTris.Add(base + triIndices[t * 3 + 2]);
				OutTris.Add(base + triIndices[t * 3 + 1]);
			}
		}
	}
}
void ADebugTransvoxel::DebugWriteLowLODDensityField2Desk(int32 Width, int32 Height)
{
	// 确保数组大小足够（行优先：index = y * Width + x）
	check(LowLODDensityField.Num() == Width * Height);

	// 构建文件路径
	FString FilePath = FPaths::ProjectSavedDir() + TEXT("DebugLog_LowLOD.txt");

	// 构建输出内容
	FString OutputString;
	OutputString.Reserve(Width * Height * 20);

	// 按X轴分层写入（行优先：外层X，内层Y）
	for (int32 X = 0; X < Width; ++X)
	{
		// 写入X坐标标识
		OutputString += FString::Printf(TEXT("体素坐标->x=%d\n"), X);

		// 遍历Y轴
		for (int32 Y = 0; Y < Height; ++Y)
		{
			// 行优先索引计算：index = y * Width + x
			int32 Index = Y * Width + X;
			float DensityValue = LowLODDensityField[Index];

			// 格式化：(x,y)[value]
			if (Y > 0)
			{
				OutputString += TEXT(",   ");
			}
			OutputString += FString::Printf(TEXT("(%d,%d)[%.3f]"), X, Y, DensityValue);
		}
		OutputString += TEXT("\n\n");  // 每行结束后加两个换行
	}

	// 写入文件
	if (FFileHelper::SaveStringToFile(OutputString, *FilePath, FFileHelper::EEncodingOptions::AutoDetect))
	{
		UE_LOG(LogTemp, Log, TEXT("低LOD密度场数据已写入: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("无法写入文件: %s"), *FilePath);
	}
}
int32 ADebugTransvoxel::CaculateLowLODDensityFieldIndex(int32 x, int32 y)
{
	int32 index = y * this->LowLODDensitySize.Y + x;
	return index;
}


// Sets default values
ADebugTransvoxel::ADebugTransvoxel()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADebugTransvoxel::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADebugTransvoxel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

