#include "PlanetActor.h"
#include "NoiseGenerator.h"
#include "PlanetChunk.h"
#include "Components/StaticMeshComponent.h"
#include "CxGamiProduralMeshComponent.h"
#include "Engine/Engine.h"
#include <cmath>
#include <thread>
APlanetActor::APlanetActor()
{
    PrimaryActorTick.bCanEverTick = false;
    
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    ChunkSize = 128.0f;  // Larger chunks like Rust (16 voxels * 8 = 128 units)
    ChunksPerAxis = 16;
    VoxelsPerChunk = 16; // Same as Rust
    bEnableCollision = false;
    
}

void APlanetActor::BeginPlay()
{
    Super::BeginPlay();
    float Size = ChunkSize * ChunksPerAxis ;
    LoadRadiu = Size * Size;
    
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
        return;
    }
    FVector ActorPosition = GetActorLocation();   
}

void APlanetActor::GenerateAllChunks(UProceduralMeshComponent* component, FPlanetChunk* planet_chunk )
{
  
    if (component)
    {
        component->ClearAllMeshSections();
    }

    // Calculate chunk bounds exactly like Rust implementation
    // Rust uses: chunks_extent = Extent3i::from_min_and_lub(IVec3::from([-5; 3]), IVec3::from([5; 3]))
    // Which creates a 10x10x10 grid centered around origin
    // float HalfExtent = (ChunksPerAxis / 2) * ChunkSize;
    FVector PlanetCenter = GetActorLocation();
   // FVector StartPosition = FVector(0.f, 0.f, 0.f); //- FVector(HalfExtent, HalfExtent, HalfExtent);
    
    //UE_LOG(LogSurfaceNets, Log, TEXT("Generating chunks from %s to %s (ChunkSize: %f)"), 
    //       *StartPosition.ToString(), 
    //       *(StartPosition + FVector(ChunksPerAxis * ChunkSize)).ToString(),
    //       ChunkSize);
    
    // Generate chunks in a grid pattern (equivalent to Rust chunks_extent.iter3())
    int32 GeneratedChunks = 0;


    if (!component)
    {
        auto Start = FPlatformTime::Seconds() * 1000.0;

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

                    if (GenerateChunk(X, Y, Z, ChunkCenter))
                    {
                        GeneratedChunks++;
                    }
                }
            }
        }
        auto End = FPlatformTime::Seconds() * 1000.0;
    }
    else
    {
    
        UCxGamiProduralMeshComponent* PMC = static_cast<UCxGamiProduralMeshComponent*>(component);
        int32 X = PMC->Location.X;
		int32 Y = PMC->Location.Y;
		int32 Z = PMC->Location.Z;
		FVector ChunkCenter = FVector(
			(X * ChunkSize) /*+ (ChunkSize * 0.5f)*/,
			(Y * ChunkSize) /*+ (ChunkSize * 0.5f)*/,
			(Z * ChunkSize) /*+ (ChunkSize * 0.5f)*/
		);
        
        GenerateChunk(X, Y, Z, ChunkCenter, component, planet_chunk);
    }
}

bool APlanetActor::GenerateChunk(int32 X, int32 Y, int32 Z, FVector ChunkCenter, UProceduralMeshComponent* component, FPlanetChunk* planet_chunk )
{
    // CRITICAL FIX: Null check before passing NoiseGenerator
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
    if (!component)
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
    if (!planet_chunk)
    {
        NewChunk = MakeUnique<FPlanetChunk>(ChunkCenter, 0, ChunkSize);
    }

    UCxGamiProduralMeshComponent* MeshComponent = nullptr;
    
    FVector LocationKey(X + ChunkLocation.X, Y + ChunkLocation.Y, Z + ChunkLocation.Z);
    this->ChunkBox.Add(LocationKey, TWeakObjectPtr<UProceduralMeshComponent>());

    
    bool bMeshGenerated = NewChunk->GenerateMesh(NoiseGenerator);
    if (bMeshGenerated && NewChunk->Vertices.Num() > 0 && NewChunk->Triangles.Num() > 0 )
    {
        // Create mesh component
        if (component)
        {
            MeshComponent = (UCxGamiProduralMeshComponent*)component;
        }
        else
        {
            MeshComponent = (UCxGamiProduralMeshComponent*)CreateMeshComponent();
            MeshComponent->Location.X = X + ChunkLocation.X;
            MeshComponent->Location.Y = Y + ChunkLocation.Y;
            MeshComponent->Location.Z = Z + ChunkLocation.Z;
            *this->ChunkBox.Find(LocationKey) = MeshComponent;
        }
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
        
        if (PlanetMaterial)
        {
            MeshComponent->SetMaterial(0, PlanetMaterial);
        }
    }

   
    if (!component)
    {
        if (MeshComponent)
        {
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

    return bMeshGenerated;
}


void APlanetActor::CircleChunk(FVector Location)
{

    if (this->ChunkBox.Find(Location) != nullptr)
    {
        auto it = this->ChunkBox.Find(Location);
        if (it->IsValid())
        {
            this->ChunkBox.Find(Location)->Get()->DestroyComponent();
            this->ChunkBox.Remove(Location);
        }
    }
}