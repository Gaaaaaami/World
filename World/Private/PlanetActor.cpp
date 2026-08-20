#include "PlanetActor.h"
#include "NoiseGenerator.h"
#include "PlanetChunk.h"
#include "SurfaceNetsUE.h"
#include "Components/StaticMeshComponent.h"
#include "CxGamiProduralMeshComponent.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include <cmath>
#include <thread>

/**
 * 将 TArray<float> 密度数据按层输出到 LOG.txt
 * @param DensityData  密度数组（按 Z->Y->X 排列）
 * @param SizeX        X 方向尺寸
 * @param SizeY        Y 方向尺寸
 * @param SizeZ        Z 方向层数
 * @param FileName     输出文件名（默认 LOG.txt）
 */
void DumpDensityToLog(
    const TArray<float>& DensityData,
    int32 SizeX,
    int32 SizeY,
    int32 SizeZ,
    const FString& FileName = TEXT("LOG")
);

void DumpDensityToLog(
    const TArray<float>& DensityData,
    int32 SizeX,
    int32 SizeY,
    int32 SizeZ,
    const FString& FileName)
{
    const int32 Expected = SizeX * SizeY * SizeZ;
    if (DensityData.Num() != Expected)
    {
        UE_LOG(LogTemp, Error, TEXT("DumpDensityToLog: 数据尺寸不对！期望 %d，实际 %d"), Expected, DensityData.Num());
        return;
    }

    FString Content;

    // 逐层输出
    for (int32 Z = 0; Z < SizeZ; ++Z)
    {
        // 层标题（第1层、第2层...）
        Content.Appendf(TEXT("第%d层:\n"), Z + 1);

        // 每一层的 Y 行
        for (int32 Y = 0; Y < SizeY; ++Y)
        {
            FString Row;
            for (int32 X = 0; X < SizeX; ++X)
            {
                // ★ 索引计算：Z -> Y -> X
                // 如果你的数据排列顺序不一样，改这一行就行
                const int32 Index = Z * (SizeX * SizeY) + Y * SizeX + X;
                const float Val = DensityData[Index];

                // 整数就输出整数，小数保留3位，完全还原你给的格式
                if (FMath::IsNearlyEqual(Val, FMath::RoundToInt(Val)))
                {
                    Row.Appendf(TEXT("%d,"), FMath::RoundToInt(Val));
                }
                else
                {
                    Row.Appendf(TEXT("%.3f,"), Val);
                }
            }
            Content.Appendf(TEXT("%s\n"), *Row);
        }

        // 层之间空一行，和你例子一致
        Content.Appendf(TEXT("\n"));
    }

    // 写入文件：项目目录/Saved/Logs/LOG.txt
    const FString FullPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Logs"), FileName + TEXT(".txt"));

    // 确保目录存在
    IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
    PF.CreateDirectoryTree(*FPaths::GetPath(FullPath));

    FFileHelper::SaveStringToFile(Content, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    UE_LOG(LogTemp, Warning, TEXT("✅ 密度数据已写入: %s"), *FullPath);
}





APlanetActor::APlanetActor()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // Create root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    
    // Match Rust example parameters more closely
    PlanetRadius = 1000.0f;
    ChunkSize = 128.0f;  // Larger chunks like Rust (16 voxels * 8 = 128 units)
    ChunksPerAxis = 16;
    VoxelsPerChunk = 16; // Same as Rust
    bEnableCollision = false;
    
    // Create noise generator
    // NoiseGenerator = CreateDefaultSubobject<UNoiseGenerator>(TEXT("NoiseGenerator"));


#if 0
    TArray<float> TestData;
    TestData.SetNum(27);

    // 第一层 (Z=0) 全 0
    for (int32 i = 0; i < 9; ++i) TestData[i] = 0.f;

    // 第二层 (Z=1) 全 0
    for (int32 i = 9; i < 18; ++i) TestData[i] = 0.f;

    // 第三层 (Z=2) 全 1
    for (int32 i = 18; i < 27; ++i) TestData[i] = 1.f;

    // 一行调用，直接出结果
    DumpDensityToLog(TestData, 3, 3, 3,"DataArray");
#endif

}

void APlanetActor::BeginPlay()
{
    Super::BeginPlay();
    float Size = ChunkSize * ChunksPerAxis ;
    LoadRadiu = Size * Size;

    UE_LOG(LogSurfaceNets, Log, TEXT("Planet spawned successfully at %s"), *GetActorLocation().ToString());
    
	if (NoiseGeneratorClass)
	{
		NoiseGenerator = NewObject<UNoiseGenerator>(
			this, NoiseGeneratorClass.Get()
		);
	}
	else
	{
		NoiseGenerator = NewObject<UNoiseGenerator>(this);
	}


    InitializePlanet();
}

UProceduralMeshComponent* APlanetActor::CreateMeshComponent()
{
    UCxGamiProduralMeshComponent* MeshComponent = NewObject<UCxGamiProduralMeshComponent>(this);
    MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
    MeshComponent->RegisterComponent();
    return MeshComponent;
}

void APlanetActor::InitializePlanet()
{
    if (!NoiseGenerator)
    {
        UE_LOG(LogSurfaceNets, Error, TEXT("Missing noise generator for planet initialization"));
        return;
    }
    
    // Set up noise generator with actor's world position as planet center
    FVector ActorPosition = GetActorLocation();
    NoiseGenerator->PlanetRadius = PlanetRadius;
    NoiseGenerator->PlanetCenter = ActorPosition;
    
    // Generate all chunks immediately
	MeshComponents.SetNum(this->ChunksPerAxis * this->ChunksPerAxis * this->ChunksPerAxis);
    ///GenerateAllChunks();
    UE_LOG(LogSurfaceNets, Log, TEXT("Planet initialized at %s with radius %f and %d chunks"), 
           *ActorPosition.ToString(), PlanetRadius, PlanetChunks.Num());
}

void APlanetActor::GenerateAllChunks(UProceduralMeshComponent* InProduralMeshComponet, FPlanetChunk* InPlanetChunk, TObjectPtr<URHIDensityTools> InRHIDT)
{
    if (InProduralMeshComponet)
    {
        InProduralMeshComponet->ClearAllMeshSections();
    }
    FVector PlanetCenter = GetActorLocation();
   
    if (!InProduralMeshComponet)
    {

        for (int32 X = -ChunksPerAxis; X < ChunksPerAxis ; X++)
        {
            for (int32 Y = -ChunksPerAxis; Y < ChunksPerAxis; Y++)
            {
                for (int32 Z = -ChunksPerAxis; Z < ChunksPerAxis; Z++)
                {
                    
                    // Calculate chunk center (equivalent to Rust chunk_min calculation)
                    FVector ChunkCenter = FVector(
						(X * ChunkSize) /*+(ChunkSize * 0.5f)*/,
						(Y * ChunkSize) /*+(ChunkSize * 0.5f)*/,
						(Z * ChunkSize) /*+(ChunkSize * 0.5f)*/
                    );

#if 0
                    if (GenerateChunk(X, Y, Z, ChunkCenter, nullptr, nullptr ,InRHIDT))
                    {
                        GeneratedChunks++;
                    }
#else

                    RHIGenerateChunk(X, Y, Z, ChunkCenter, nullptr, nullptr, InRHIDT);

#endif
                }
            }
        }
    }
    else
    {
    
        UCxGamiProduralMeshComponent* PMC = static_cast<UCxGamiProduralMeshComponent*>(InProduralMeshComponet);
        int32 X = PMC->Location.X;
		int32 Y = PMC->Location.Y;
		int32 Z = PMC->Location.Z;
		FVector ChunkCenter = FVector(
			(X * ChunkSize) /*+ (ChunkSize * 0.5f)*/,
			(Y * ChunkSize) /*+ (ChunkSize * 0.5f)*/,
			(Z * ChunkSize) /*+ (ChunkSize * 0.5f)*/
		);
        
        GenerateChunk(X, Y, Z, ChunkCenter, InProduralMeshComponet, InPlanetChunk, InRHIDT);
    }

}

bool APlanetActor::GenerateChunk(int32 X, int32 Y, int32 Z, FVector ChunkCenter, UProceduralMeshComponent* InProduralMeshComponet, FPlanetChunk* InPlanetChunk, TObjectPtr<URHIDensityTools> InRHIDT)
{
    if (!NoiseGenerator)
    {
        return false;
    }
    FVector ChunkLocation = this->PlayerLocation / ChunkSize;
    ChunkLocation.X = std::floor(ChunkLocation.X);
    ChunkLocation.Y = std::floor(ChunkLocation.Y);
    ChunkLocation.Z = std::floor(ChunkLocation.Z);
    FVector Local((X + ChunkLocation.X) * ChunkSize,
                  (Y + ChunkLocation.Y) * ChunkSize,
                  (Z + ChunkLocation.Z) * ChunkSize);
    if (!InProduralMeshComponet)
    {
        float R = this->LoadRadiu;
        ChunkCenter += FVector(ChunkLocation.X * ChunkSize,
                               ChunkLocation.Y * ChunkSize, 
                               ChunkLocation.Z * ChunkSize);

		float DistSquared = FVector::DistSquared(Local, this->PlayerLocation);
		float Alpha = DistSquared / R;
		if (Alpha >= 1.f || this->ChunkBox.Contains(FVector(X + ChunkLocation.X, Y + ChunkLocation.Y, Z + ChunkLocation.Z)))
		{
			return false;
		}
    }
    
    TUniquePtr<FPlanetChunk> NewChunk = nullptr;    
    TUniquePtr<FPlanetChunk>* ContainerOfNewChunk = nullptr;
    if (!InPlanetChunk)
    {
        NewChunk = MakeUnique<FPlanetChunk>(ChunkCenter, 0, ChunkSize);
    }
    else
    {
        for (auto& it : this->PlanetChunks)
		{
			if (it.Get() == InPlanetChunk)
			{
				NewChunk = MoveTemp(it);
                ContainerOfNewChunk = &it;
				break;
			}
		}
    }

    UCxGamiProduralMeshComponent* MeshComponent = nullptr;
	// Create mesh component
	if (InProduralMeshComponet)
	{
		MeshComponent = (UCxGamiProduralMeshComponent*)InProduralMeshComponet;
	}
	else
	{
        MeshComponent = (UCxGamiProduralMeshComponent*)CreateMeshComponent();
		MeshComponent->Location.X = X + ChunkLocation.X;
		MeshComponent->Location.Y = Y + ChunkLocation.Y;
		MeshComponent->Location.Z = Z + ChunkLocation.Z;
		FVector LocationKey(MeshComponent->Location.X, MeshComponent->Location.Y, MeshComponent->Location.Z);
		this->ChunkBox.Add(LocationKey, MeshComponent);
	}
    
    // Generate mesh using the chunk's GenerateMesh method (equivalent to Rust generate_and_process_chunk)
    bool bMeshGenerated = NewChunk->GenerateMesh(NoiseGenerator, this, InRHIDT);
    
    // Only create mesh component if chunk has valid mesh data (like Rust early return)
    if (bMeshGenerated && NewChunk->Vertices.Num() > 0 && NewChunk->Triangles.Num() > 0 )
    {

        // Create mesh section using the chunk's mesh data
        TArray<FColor> VertexColors;
        TArray<FProcMeshTangent> Tangents;
        MeshComponent->CreateMeshSection(
            0,
            NewChunk->Vertices,
            NewChunk->Triangles,
            NewChunk->Normals,
            NewChunk->UVs,
            VertexColors,
            Tangents,
            bEnableCollision
        );
        
        // Apply material
        if (PlanetMaterial)
        {
            MeshComponent->SetMaterial(0, PlanetMaterial);
        }
        
        // Store mesh component reference
    }

    // Always store the chunk (even if it has no mesh) for consistency

    if (!InProduralMeshComponet)
    {
        PlanetChunks.Add(MoveTemp(NewChunk));
        if (MeshComponent)
        {
            MeshComponent->BindPlanetChunk(PlanetChunks.Last().Get());
            MeshComponent->BindPlanetActor(this);
        }

		{
			int idx = Z * (ChunksPerAxis * ChunksPerAxis) + Y * ChunksPerAxis + X;
			if (MeshComponents.IsValidIndex(idx))
			{
				MeshComponents[idx] = MeshComponent;   
			}
		}
    }

    if (ContainerOfNewChunk)
    {
        *ContainerOfNewChunk = MoveTemp(NewChunk);
    }
    return bMeshGenerated;
}

bool APlanetActor::RHIGenerateChunk(int32 X, int32 Y, int32 Z, FVector ChunkCenter, UProceduralMeshComponent* InProduralMeshComponet, FPlanetChunk* InPlanetChunk, TObjectPtr<URHIDensityTools> InRHIDT)
{

    FVector ChunkLocation = this->PlayerLocation / ChunkSize;
    ChunkLocation.X = std::floor(ChunkLocation.X);
    ChunkLocation.Y = std::floor(ChunkLocation.Y);
    ChunkLocation.Z = std::floor(ChunkLocation.Z);
    FVector Local((X + ChunkLocation.X) * ChunkSize,
        (Y + ChunkLocation.Y) * ChunkSize,
        (Z + ChunkLocation.Z) * ChunkSize);
    if (!InProduralMeshComponet)
    {
        float R = this->LoadRadiu;
        ChunkCenter += FVector(ChunkLocation.X * ChunkSize,
            ChunkLocation.Y * ChunkSize,
            ChunkLocation.Z * ChunkSize);

        float DistSquared = FVector::DistSquared(Local, this->PlayerLocation);
        float Alpha = DistSquared / R;

        auto it = this->ChunkBox.Find(FVector(X + ChunkLocation.X, Y + ChunkLocation.Y, Z + ChunkLocation.Z));
        if (Alpha >= 1.f || it)
        {
            return false;
        }
    }

    URHIDensityTools::stRenderTargetCommand command;
    command.Location.X = X + ChunkLocation.X;
    command.Location.Y = Y + ChunkLocation.Y;
    command.Location.Z = Z + ChunkLocation.Z;
    command.ChunkCenter = ChunkCenter;
    this->ChunkBox.Add(FVector(command.Location), TWeakObjectPtr<UProceduralMeshComponent>());
    InRHIDT->InsertCommand(command);

#if 0

    TUniquePtr<FPlanetChunk> NewChunk = nullptr;
    TUniquePtr<FPlanetChunk>* ContainerOfNewChunk = nullptr;
    if (!InPlanetChunk)
    {
        NewChunk = MakeUnique<FPlanetChunk>(ChunkCenter, 0, ChunkSize);
    }
    else
    {
        for (auto& it : this->PlanetChunks)
        {
            if (it.Get() == InPlanetChunk)
            {
                NewChunk = MoveTemp(it);
                ContainerOfNewChunk = &it;
                break;
            }
        }
    }

    UCxGamiProduralMeshComponent* MeshComponent = nullptr;
    if (InProduralMeshComponet)
    {
        MeshComponent = (UCxGamiProduralMeshComponent*)InProduralMeshComponet;
    }
    else
    {
        this->PlanetChunks.Push(MoveTemp(NewChunk));
        MeshComponent = (UCxGamiProduralMeshComponent*)CreateMeshComponent();
        MeshComponent->Location.X = X + ChunkLocation.X;
        MeshComponent->Location.Y = Y + ChunkLocation.Y;
        MeshComponent->Location.Z = Z + ChunkLocation.Z;

        MeshComponent->BindPlanetActor(this);
        MeshComponent->BindPlanetChunk(this->PlanetChunks.Last().Get());
        FVector LocationKey(MeshComponent->Location.X, MeshComponent->Location.Y, MeshComponent->Location.Z);
        this->ChunkBox.Add(LocationKey, MeshComponent);
        // 数据插入渲染池
    
   
    }
#endif





    return true;
}

bool APlanetActor::GenerateMesh(const TArray<float>& InDensityField, FVector3f InLocation  , FVector InChunkCenter)
{
   FVector LocationKey(InLocation);
   TUniquePtr<FPlanetChunk> NewChunk = MakeUnique<FPlanetChunk>(InChunkCenter, 0, ChunkSize);
   bool bMeshGenerated = NewChunk->RHIGenerateMesh(InDensityField, this);

   // Only create mesh component if chunk has valid mesh data (like Rust early return)
   if (bMeshGenerated && NewChunk->Vertices.Num() > 0 && NewChunk->Triangles.Num() > 0)
   {
       UCxGamiProduralMeshComponent* MeshComponent = (UCxGamiProduralMeshComponent *)CreateMeshComponent();
       MeshComponent->BindPlanetActor(this);
       MeshComponent->Location = LocationKey;


       if (ChunkBox.Find(LocationKey))
       {
           this->ChunkBox[LocationKey] = MeshComponent;
       }

       TArray<FColor> VertexColors;
       TArray<FProcMeshTangent> Tangents;
       MeshComponent->CreateMeshSection(
           0,
           NewChunk->Vertices,
           NewChunk->Triangles,
           NewChunk->Normals,
           NewChunk->UVs,
           VertexColors,
           Tangents,
           bEnableCollision
       );

    }

   return true;

}

void APlanetActor::LogPlanetStats()
{
    int32 TotalVertices = 0;
    int32 TotalTriangles = 0;
    
    for (const auto& Chunk : PlanetChunks)
    {
        if (Chunk.IsValid())
        {
            TotalVertices += Chunk->Vertices.Num();
            TotalTriangles += Chunk->Triangles.Num() / 3;
        }
    }
    
    UE_LOG(LogSurfaceNets, Warning, TEXT("Planet Stats:"));
    UE_LOG(LogSurfaceNets, Warning, TEXT("  Generated Chunks: %d"), PlanetChunks.Num());
    UE_LOG(LogSurfaceNets, Warning, TEXT("  Active Mesh Components: %d"), MeshComponents.Num());
    UE_LOG(LogSurfaceNets, Warning, TEXT("  Total Vertices: %d"), TotalVertices);
    UE_LOG(LogSurfaceNets, Warning, TEXT("  Total Triangles: %d"), TotalTriangles);
    UE_LOG(LogSurfaceNets, Warning, TEXT("  Planet Radius: %f"), PlanetRadius);
    UE_LOG(LogSurfaceNets, Warning, TEXT("  Chunk Size: %f"), ChunkSize);
}

void APlanetActor::CircleChunk(FVector Location)
{

    if (this->ChunkBox.Find(Location) != nullptr)
    {
        this->ChunkBox.Find(Location)->Get()->DestroyComponent();
        this->ChunkBox.Remove(Location);
    }
}