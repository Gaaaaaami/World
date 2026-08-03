// Fill out your copyright notice in the Description page of Project Settings.

#include "CxDynamicLandScape.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Math/UnrealMathUtility.h"
#include "StaticMeshOperations.h"
#include "brutus/brutus.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"       // 必须加，TActorIterator 需要
#include "Components/LightComponent.h"

#if 1
/* =================== SurfaceNets 核心算法 =================== */
typedef struct {
	float x, y, z;
} float3D;
typedef struct {
	int a, b, c;
} int3D;
typedef struct {
	int np, nt;
	float3D* p;
	int3D* t;
} Mesh;

int cube_edges[24];
int edge_table[256];

void init_surfacenets(void)
{
	int i, j, p, em, k = 0;
	for (i = 0; i < 8; ++i) {
		for (j = 1; j <= 4; j = j << 1) {
			p = i ^ j;
			if (i <= p) {
				cube_edges[k++] = i;
				cube_edges[k++] = p;
			}
		}
	}
	for (i = 0; i < 256; ++i) {
		em = 0;
		for (j = 0; j < 24; j += 2) {
			int a = !(i & (1 << cube_edges[j]));
			int b = !(i & (1 << cube_edges[j + 1]));
			em |= a != b ? (1 << (j >> 1)) : 0;
		}
		edge_table[i] = em;
	}
}

void SurfaceNets(float* data, int* dims, float level, Mesh* mesh, int storeFlag)
{
	float3D* vertices = mesh->p;
	int3D* faces = mesh->t;
	int n = 0;
	float x[3];
	int R[3];
	float* grid = (float*)calloc(8, sizeof(float));
	int buf_no = 1;
	int* buffer = nullptr;
	int buffer_length = 0;
	int vertices_length = 0;
	int faces_length = 0;
	int	i, j, k;

	R[0] = 1;
	R[1] = dims[0] + 1;
	R[2] = (dims[0] + 1) * (dims[1] + 1);

	if (R[2] * 2 > buffer_length)
		buffer = (int*)calloc(R[2] * 2, sizeof(int));
	else
		return;

	for (x[2] = 0; x[2] < dims[2] - 1; ++x[2])
	{
		int m = 1 + (dims[0] + 1) * (1 + buf_no * (dims[1] + 1));
		for (x[1] = 0; x[1] < dims[1] - 1; ++x[1], ++n, m += 2)
			for (x[0] = 0; x[0] < dims[0] - 1; ++x[0], ++n, ++m)
			{
				int mask = 0, g = 0, idx = n;
				for (k = 0; k < 2; ++k, idx += dims[0] * (dims[1] - 2))
					for (j = 0; j < 2; ++j, idx += dims[0] - 2)
						for (i = 0; i < 2; ++i, ++g, ++idx)
						{
							float p = data[idx] - level;
							grid[g] = p;
							mask |= (p < 0) ? (1 << g) : 0;
						}
				if (mask == 0 || mask == 0xff)
					continue;

				int edge_mask = edge_table[mask];
				float3D v = { 0.0,0.0,0.0 };
				int e_count = 0;
				for (i = 0; i < 12; ++i)
				{
					if (!(edge_mask & (1 << i))) continue;
					++e_count;
					int e0 = cube_edges[i << 1];
					int e1 = cube_edges[(i << 1) + 1];
					float g0 = grid[e0];
					float g1 = grid[e1];
					float t = g0 - g1;
					if (fabs(t) > 1e-6) t = g0 / t; else continue;
					k = 1;
					for (j = 0; j < 3; ++j)
					{
						int a = e0 & k, b = e1 & k;
						if (a != b) ((float*)&v)[j] += a ? 1.0 - t : t;
						else ((float*)&v)[j] += a ? 1.0 : 0;
						k = k << 1;
					}
				}

				float s = 1.0 / (e_count > 0 ? e_count : 1);
				for (i = 0; i < 3; ++i)
					((float*)&v)[i] = x[i] + s * ((float*)&v)[i];

				buffer[m] = vertices_length;
				if (storeFlag)
					vertices[vertices_length++] = v;
				else
					vertices_length++;

				for (i = 0; i < 3; ++i)
				{
					if (!(edge_mask & (1 << i))) continue;
					int iu = (i + 1) % 3, iv = (i + 2) % 3;
					if (x[iu] == 0 || x[iv] == 0) continue;
					int du = R[iu], dv = R[iv];
					if (storeFlag)
					{
						if (mask & 1)
						{
							faces[faces_length++] = int3D{ buffer[m], buffer[m - du - dv], buffer[m - du] };
							faces[faces_length++] = int3D{ buffer[m], buffer[m - dv], buffer[m - du - dv] };
						}
						else
						{
							faces[faces_length++] = int3D{ buffer[m], buffer[m - du - dv], buffer[m - dv] };
							faces[faces_length++] = int3D{ buffer[m], buffer[m - du], buffer[m - du - dv] };
						}
					}
					else
						faces_length += 2;
				}
			}
		n += dims[0];
		buf_no ^= 1;
		R[2] = -R[2];
	}

	// 【修复】释放临时缓冲区，防止内存泄漏
	if (buffer) free(buffer);
	if (grid) free(grid);

	mesh->np = vertices_length;
	mesh->nt = faces_length;
}

#else
#endif

/* =================== Actor 实现 =================== */

ACxDynamicLandScape::ACxDynamicLandScape()
{
	PrimaryActorTick.bCanEverTick = true;
	m_procedural_mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = m_procedural_mesh;

}

void ACxDynamicLandScape::BeginPlay()
{
	Super::BeginPlay();

}

void ACxDynamicLandScape::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!m_initialize)
	{
		m_procedural_mesh->SetCastShadow(true);
		//m_procedural_mesh->bAffectDistanceFieldLighting = false;
		m_procedural_mesh->bAffectDynamicIndirectLighting = false;
		//m_procedural_mesh->bCastDynamicShadow = false;
		init_surfacenets();
		m_initialize = true;
	}
	ResetLandscape();
}

void ACxDynamicLandScape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

/* ========== 密度场生成 ========== */
void ACxDynamicLandScape::GenerateDensityField(FVector dimSize)
{
	m_DensityData.SetNumUninitialized((dimSize.X + 1) * 
		(dimSize.Y + 1) * (dimSize.Z + 1));

	// 这块 Actor 在世界里的真实偏移（假设多块拼接时每块 ActorLocation 已对齐到网格）
	FVector worldOffset = this->GetActorLocation();

	const float noise_scale = 0.0002f;  // 调小，让噪声频率合理
	const float noise_height = NoiseHeight;

	for (int x = 0; x < dimSize.X + 1; ++x)
	{
		for (int y = 0; y < dimSize.Y + 1; ++y)
		{
			// 关键：用全局世界坐标，不再乘 0.01 这种压缩系数
			float WorldX = worldOffset.X + x * VoxelSize;
			float WorldY = worldOffset.Y + y * VoxelSize;

			for (int z = 0; z < dimSize.Z + 1; ++z)
			{

				int32 idx = z * ((dimSize.X + 1) * (dimSize.Y+1)) + y * (dimSize.X+1) + x;
				float depth = FMath::PerlinNoise2D(
					FVector2D(WorldX * noise_scale, WorldY * noise_scale)) * noise_height;
				depth = z - depth;
				m_DensityData[idx] = depth;

			}
		}
	}
}

/**
 * 带特征保持的平滑
 * @param AngleThresholdDeg - 法线夹角阈值，超过这个角度认为是"锐边"，不参与平滑（推荐 25~40度）
 */
static void ApplyFeaturePreservingSmooth(
	TArray<FVector>& Verts,
	TArray<int32>& Tris,
	int32 Iterations = 3,
	float Strength = 0.5f,
	float AngleThresholdDeg = 35.0f)
{
	if (Verts.Num() == 0) return;

	// ---- 1. 计算面法线并累加到顶点，得到初始顶点法线 ----
	TArray<FVector> VNormals;
	VNormals.Init(FVector::ZeroVector, Verts.Num());
	for (int i = 0; i < Tris.Num(); i += 3)
	{
		FVector A = Verts[Tris[i]];
		FVector B = Verts[Tris[i + 1]];
		FVector C = Verts[Tris[i + 2]];
		FVector FaceN = FVector::CrossProduct((B - A).GetSafeNormal(), (C - A).GetSafeNormal());
		if (FaceN.IsNearlyZero()) continue;
		FaceN.Normalize();
		VNormals[Tris[i]] += FaceN;
		VNormals[Tris[i + 1]] += FaceN;
		VNormals[Tris[i + 2]] += FaceN;
	}
	for (auto& n : VNormals) { if (!n.IsNearlyZero()) n.Normalize(); }

	// ---- 2. 建邻接表 ----
	TArray<TSet<int32>> Neighbors;
	Neighbors.SetNum(Verts.Num());
	for (int i = 0; i < Tris.Num(); i += 3)
	{
		int32 a = Tris[i], b = Tris[i + 1], c = Tris[i + 2];
		Neighbors[a].Add(b); Neighbors[a].Add(c);
		Neighbors[b].Add(a); Neighbors[b].Add(c);
		Neighbors[c].Add(a); Neighbors[c].Add(b);
	}

	// ---- 3. 加权迭代平滑 ----
	float CosThreshold = FMath::Cos(FMath::DegreesToRadians(AngleThresholdDeg));

	for (int iter = 0; iter < Iterations; ++iter)
	{
		TArray<FVector> NewVerts = Verts;
		for (int i = 0; i < Verts.Num(); ++i)
		{
			if (Neighbors[i].Num() == 0 || VNormals[i].IsNearlyZero()) continue;

			FVector Smoothed = FVector::ZeroVector;
			float TotalWeight = 0.f;

			for (int32 nid : Neighbors[i])
			{
				// 核心：用法线夹角做权重
				float Dot = FVector::DotProduct(VNormals[i], VNormals[nid]);
				float Weight = FMath::Max(0.f, (Dot - CosThreshold) / (1.f - CosThreshold + 0.001f));
				// 距离也作为隐式权重（天然在 TSet 里所有邻居权重一致，这里体现夹角）
				Smoothed += Verts[nid] * Weight;
				TotalWeight += Weight;
			}

			if (TotalWeight > 0.01f)
			{
				Smoothed /= TotalWeight;
				// 只沿法线垂直方向移动（防止体积收缩得太厉害）
				FVector Delta = Smoothed - Verts[i];
				NewVerts[i] = Verts[i] + Delta * Strength * (1.f / (iter + 1));
			}
		}
		Verts = MoveTemp(NewVerts);

		// （可选）每轮迭代后简单刷新一下法线，让下一轮更准确
		// 这里为了性能没每轮刷，一般够用了
	}
}
/* ========== 网格提取与渲染 ========== */
/**
 * 约束平滑（Constrained Smoothing）
 * @param Vertices   顶点数组（世界坐标，会被原地修改）
 * @param Triangles  三角形索引（每3个一组）
 * @param VoxelSize  体素大小（用于计算约束范围）
 * @param Iterations 迭代次数（5~10 效果就很好）
 */
void ApplyConstrainedSmoothing(
	TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	float VoxelSize,
	int32 Iterations = 8)
{
	if (Vertices.Num() == 0) return;

	// ========== 1. 构建邻接表 ==========
	TArray<TArray<int32>> Neighbors;
	Neighbors.SetNum(Vertices.Num());
	for (int32 i = 0; i < Triangles.Num(); i += 3)
	{
		int32 a = Triangles[i], b = Triangles[i + 1], c = Triangles[i + 2];
		Neighbors[a].AddUnique(b); Neighbors[a].AddUnique(c);
		Neighbors[b].AddUnique(a); Neighbors[b].AddUnique(c);
		Neighbors[c].AddUnique(a); Neighbors[c].AddUnique(b);
	}

	// ========== 2. 记录每个顶点初始所在的体素格子 ==========
	// 这是"约束"的核心：顶点只能待在自己出生的格子里
	TArray<FIntVector> Cells;
	Cells.SetNum(Vertices.Num());
	for (int32 i = 0; i < Vertices.Num(); i++)
	{
		Cells[i] = FIntVector(
			FMath::FloorToInt(Vertices[i].X / VoxelSize),
			FMath::FloorToInt(Vertices[i].Y / VoxelSize),
			FMath::FloorToInt(Vertices[i].Z / VoxelSize)
		);
	}

	// ========== 3. 迭代松弛 ==========
	const float StepSize = 0.5f; // 每次移动一半距离，稳定收敛

	for (int32 iter = 0; iter < Iterations; iter++)
	{
		TArray<FVector> NewPos;
		NewPos.SetNum(Vertices.Num());

		for (int32 i = 0; i < Vertices.Num(); i++)
		{
			if (Neighbors[i].Num() == 0)
			{
				NewPos[i] = Vertices[i];
				continue;
			}

			// --- 邻居平均位置 ---
			FVector Avg(0, 0, 0);
			for (int32 n : Neighbors[i])
				Avg += Vertices[n];
			Avg /= Neighbors[i].Num();

			// --- 向平均位置移动 ---
			FVector Delta = (Avg - Vertices[i]) * StepSize;
			FVector Pos = Vertices[i] + Delta;

			// --- ★ 约束：锁死在原始体素格子内 ---
			FVector CellMin(
				Cells[i].X * VoxelSize,
				Cells[i].Y * VoxelSize,
				Cells[i].Z * VoxelSize
			);
			FVector CellMax = CellMin + FVector(VoxelSize, VoxelSize, VoxelSize);
			Pos.X = FMath::Clamp(Pos.X, CellMin.X, CellMax.X);
			Pos.Y = FMath::Clamp(Pos.Y, CellMin.Y, CellMax.Y);
			Pos.Z = FMath::Clamp(Pos.Z, CellMin.Z, CellMax.Z);

			NewPos[i] = Pos;
		}

		Vertices = MoveTemp(NewPos); // 写回
	}
}
void ACxDynamicLandScape::UpdateMeshFromDensity()
{
	if (m_DensityData.Num() == 0) return;

	int dims[3] = { LandscapeDimension.X + 1, LandscapeDimension.Y + 1, LandscapeDimension.Z + 1 };


	TArray<FVector> OutVerts;
	TArray<int32> OutTris;
	TArray<FVector> normals;
	TArray<FVector2D> uvs;
	TArray<FColor> vertex_colors;
	TArray<FProcMeshTangent> tangents;

	Mesh m = { 0 };
	SurfaceNets(m_DensityData.GetData(), dims, 0.0f, &m, 0);
	if (m.np <= 0) return;

	m.p = (float3D*)malloc(m.np * sizeof(float3D));
	m.t = (int3D*)malloc(m.nt * sizeof(int3D));
	SurfaceNets(m_DensityData.GetData(), dims, 0.0f, &m, 1);

	const float Scale = this->VoxelSize;
	OutVerts.Reserve(m.np);
	for (int i = 0; i < m.np; ++i)
	{
		OutVerts.Add(FVector(m.p[i].x * Scale, m.p[i].y * Scale, m.p[i].z * Scale) /*- FVector(VoxelSize / 2., VoxelSize / 2., 0.)*/);
		uvs.Add(FVector2D(m.p[i].x * 0.005f, m.p[i].y * 0.005f));
	}

	OutTris.Reserve(m.nt * 3);
	for (int i = 0; i < m.nt; ++i)
	{
		OutTris.Add(m.t[i].a);
		OutTris.Add(m.t[i].b);
		OutTris.Add(m.t[i].c);
	}
	// 在 free(m.p) 之前，把顶点和索引传进去
	// ApplyFeaturePreservingSmooth(OutVerts, OutTris, 3, 0.45f, 30.0f);
	free(m.p);
	free(m.t);
	ApplyConstrainedSmoothing(OutVerts, OutTris, VoxelSize, 8);

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(OutVerts, OutTris, uvs, normals, tangents);	/** < 计算tangents、normals */
	
	
	m_procedural_mesh->CreateMeshSection(0, OutVerts, OutTris, normals, uvs, vertex_colors, tangents, true);
	if (VoxelMaterial)
		m_procedural_mesh->SetMaterial(0, VoxelMaterial);


}

/* ========== 对外接口 ========== */
void ACxDynamicLandScape::ResetLandscape()
{
	GenerateDensityField(LandscapeDimension);
	UpdateMeshFromDensity();
}

void ACxDynamicLandScape::DigHoleAtWorldLocation(FVector WorldLocation, float BrushRadiusCm)
{

#if 0
	if (m_DensityData.Num() == 0) return;

	// 1. 坐标转换（别用 floor，保留浮点精度做距离）
	FVector CenterLocal = GetActorTransform().InverseTransformPosition(WorldLocation) / VoxelSize;
	float RadiusInVoxels = (BrushRadiusCm / VoxelSize);

	int32 DimX = static_cast<int32>(LandscapeDimension.X);
	int32 DimY = static_cast<int32>(LandscapeDimension.Y);
	int32 DimZ = static_cast<int32>(LandscapeDimension.Z);

	// 只遍历影响球范围内的格子，别傻傻扫全图
	int32 CX = FMath::FloorToInt(CenterLocal.X);
	int32 CY = FMath::FloorToInt(CenterLocal.Y);
	int32 CZ = FMath::FloorToInt(CenterLocal.Z);
	int32 R = FMath::CeilToInt(RadiusInVoxels);

	for (int32 z = FMath::Max(0, CZ - R); z <= FMath::Min(DimZ - 1, CZ + R); ++z)
		for (int32 y = FMath::Max(0, CY - R); y <= FMath::Min(DimY - 1, CY + R); ++y)
			for (int32 x = FMath::Max(0, CX - R); x <= FMath::Min(DimX - 1, CX + R); ++x)
			{
				// 欧氏距离
				float Dist = FVector::Dist(FVector(x, y, z), CenterLocal);
				if (Dist > RadiusInVoxels) continue;

				// 关键：用距离做衰减，制造连续密度差
				float Falloff = 1.0f - (Dist / RadiusInVoxels);
				Falloff = std::pow(Falloff, FadeStrength);
				int32 idx = z * (DimX * DimY) + y * DimX + x;

				// 越靠近中心越"空"（用 Max 保证不回填）
				m_DensityData[idx] = FMath::Max(m_DensityData[idx], Falloff );
			}
#else

	if (m_DensityData.Num() == 0) return;

	// 1. 坐标转换（别用 floor，保留浮点精度做距离）
	FVector CenterLocal = GetActorTransform().InverseTransformPosition(WorldLocation) / VoxelSize;


	
#endif
	UpdateMeshFromDensity();
}

/**
 * 有向椭球体挖掘（支持任意方向）
 * @param WorldLocation 打击点
 * @param BrushRadiusCm 截面半径（粗细）
 * @param Strength       挖掘强度（决定拉长多少，即"钻头"长度）
 * @param Direction      挖掘方向（必须归一化，比如摄像机前向、或法线反方向）
 */
void ACxDynamicLandScape::DigHoleAtWorldLocation_Directed(
	FVector WorldLocation,
	float BrushRadiusCm,
	float Strength /*0~1*/,
	FVector Direction)
{
	if (m_DensityData.Num() == 0 || BrushRadiusCm <= KINDA_SMALL_NUMBER) return;

	Strength = FMath::Clamp(Strength, 0.1f, 1.0f);
	Direction = Direction.GetSafeNormal();

	// 1. 空间转换 & 偏移补偿
	FVector LocalHit = GetActorTransform().InverseTransformPosition(WorldLocation);
	FVector CenterLocal = LocalHit + FVector(VoxelSize * 0.5f, VoxelSize * 0.5f, 0.f);

	// 【关键修正】把起始中心沿着反方向退一点，让坑看起来是完整的半球/锥状，而不是被地表切了一半
	FVector CenterLocalOffset = CenterLocal - Direction * (BrushRadiusCm * 0.3f);
	FVector CenterVoxel = CenterLocalOffset / VoxelSize;

	// 2. 椭球参数
	float RadiusInVoxels = BrushRadiusCm / VoxelSize;
	float HalfLengthInVoxels = RadiusInVoxels * FMath::Lerp(0.8f, 5.0f, Strength); // 顺着方向延伸的长度
	int32 SearchRange = FMath::CeilToInt(HalfLengthInVoxels + RadiusInVoxels);

	int32 CX = FMath::FloorToInt(CenterVoxel.X);
	int32 CY = FMath::FloorToInt(CenterVoxel.Y);
	int32 CZ = FMath::FloorToInt(CenterVoxel.Z);

	int32 DimX = static_cast<int32>(LandscapeDimension.X);
	int32 DimY = static_cast<int32>(LandscapeDimension.Y);
	int32 DimZ = static_cast<int32>(LandscapeDimension.Z);

	for (int32 z = FMath::Max(0, CZ - SearchRange); z <= FMath::Min(DimZ - 1, CZ + SearchRange); ++z)
	{
		for (int32 y = FMath::Max(0, CY - SearchRange); y <= FMath::Min(DimY - 1, CY + SearchRange); ++y)
		{
			for (int32 x = FMath::Max(0, CX - SearchRange); x <= FMath::Min(DimX - 1, CX + SearchRange); ++x)
			{
				FVector VoxelCenter((float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f);
				FVector Diff = VoxelCenter - CenterVoxel;

				// 投影
				float ProjAlong = FVector::DotProduct(Diff, Direction);

				// ====== 核心修复：只保留前方，丢弃后方 ======
				if (ProjAlong < 0.0f) continue;
				// ==========================================

				FVector ProjPerp = Diff - ProjAlong * Direction;

				// 单向椭球方程：(垂直距离/半径)^2 + (轴向距离/长度)^2
				float RadialTerm = ProjPerp.SizeSquared() / (RadiusInVoxels * RadiusInVoxels);
				float AxialTerm = (ProjAlong * ProjAlong) / (HalfLengthInVoxels * HalfLengthInVoxels);
				float EllipsoidDist = RadialTerm + AxialTerm;

				if (EllipsoidDist > 1.0f) continue;

				int32 idx = z * (DimX * DimY) + y * DimX + x;
				if (!m_DensityData.IsValidIndex(idx)) continue;

				// 越靠近钻头轴心/尖端，Falloff 越高
				float Falloff = FMath::Pow(1.0f - EllipsoidDist, 1.5f);
				float TargetDensity = Falloff * Strength * 2.5f;

				// 只挖不填（推成正数变空气）
				m_DensityData[idx] = FMath::Max(m_DensityData[idx], TargetDensity);
			}
		}
	}

	UpdateMeshFromDensity();
}




void ACxDynamicLandScape::DigHoleAtWorldUnitLocation(FVector WorldLocation)
{
	FVector relative_location = WorldLocation + this->GetActorLocation();
	int x = std::floor(relative_location.X / this->VoxelSize);
	int y = std::floor(relative_location.Y / this->VoxelSize);
	int z = std::floor(relative_location.Z / this->VoxelSize);

	UE_LOG(LogTemp, Log, TEXT("x = %d|y = %d|z = %d"), x, y, z);



}