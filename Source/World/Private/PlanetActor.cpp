#include "PlanetActor.h"
#include "NoiseGenerator.h"
#include "PlanetChunk.h"
#include "SurfaceNetsUE.h"
#include "Components/StaticMeshComponent.h"
#include "CxGamiProduralMeshComponent.h"
#include "Engine/Engine.h"
#include "SPSCQueue.h"
#include <cmath>
#include <thread>
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
		// Ĭ�϶���
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
	//MeshComponents.SetNum(this->ChunksPerAxis * this->ChunksPerAxis * this->ChunksPerAxis);
    ///GenerateAllChunks();
    UE_LOG(LogSurfaceNets, Log, TEXT("Planet initialized at %s with radius %f and %d chunks"), 
           *ActorPosition.ToString(), PlanetRadius, PlanetChunks.Num());




}

void APlanetActor::GenerateAllChunks()
{
}

bool APlanetActor::GenerateChunk(int32 X, int32 Y, int32 Z, FVector ChunkCenter, FVector InSize, TArray<float>& InDensity, FVector InDensitySize)
{
    if (!NoiseGenerator)
    {
        return false;
    }
    NoiseGenerator->Density = InDensity;
    NoiseGenerator->DensitySize = InDensitySize;

    FVector ChunkLocation = this->PlayerLocation / InSize;
    ChunkLocation.X = std::floor(ChunkLocation.X);
    ChunkLocation.Y = std::floor(ChunkLocation.Y);
    ChunkLocation.Z = std::floor(ChunkLocation.Z);
    FVector Local((X + ChunkLocation.X),
        (Y + ChunkLocation.Y),
        (Z + ChunkLocation.Z));
    Local = Local * InSize;

    TUniquePtr<FPlanetChunk> NewChunk = nullptr;    
    NewChunk = MakeUnique<FPlanetChunk>(ChunkCenter, 0, InSize, ChunkSize);

    UCxGamiProduralMeshComponent* MeshComponent = nullptr;
    MeshComponent = (UCxGamiProduralMeshComponent*)CreateMeshComponent();
    MeshComponent->Location.X = X + ChunkLocation.X;
    MeshComponent->Location.Y = Y + ChunkLocation.Y;
    MeshComponent->Location.Z = Z + ChunkLocation.Z;


    FVector LocationKEY(MeshComponent->Location.X, MeshComponent->Location.Y, MeshComponent->Location.Z);
    this->ChunkBox.Add(LocationKEY, MeshComponent);

    bool bMeshGenerated = NewChunk->GenerateMesh(NoiseGenerator);
    if (bMeshGenerated && NewChunk->Vertices.Num() > 0 && NewChunk->Triangles.Num() > 0 )
    {

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
        
        if (PlanetMaterial)
        {
            MeshComponent->SetMaterial(0, PlanetMaterial);
        }
        
    }
 
    return bMeshGenerated;
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