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

void APlanetActor::GenerateAllChunks()
{
    const FVector PlanetCenter = GetActorLocation();

    for (int32 X = -ChunksPerAxis; X < ChunksPerAxis; X++)
    {
        for (int32 Y = -ChunksPerAxis; Y < ChunksPerAxis; Y++)
        {
            for (int32 Z = -ChunksPerAxis; Z < ChunksPerAxis; Z++)
            {
                FVector ChunkCenter = FVector(
                    (X * ChunkSize),
                    (Y * ChunkSize),
                    (Z * ChunkSize)
                );
                GenerateChunk(X, Y, Z, ChunkCenter);
            }
        }
    }
}

bool APlanetActor::GenerateChunk(int32 X, int32 Y, int32 Z, FVector ChunkCenter, int32 PaddingSize)
{
    //TODO
    return false;
 //   if (!NoiseGenerator)
 //   {
 //       return false;
 //   }
 //   FVector ChunkLocation = this->PlayerLocation / ChunkSize;
 //   ChunkLocation.X = std::floor(ChunkLocation.X);
 //   ChunkLocation.Y = std::floor(ChunkLocation.Y);
 //   ChunkLocation.Z = std::floor(ChunkLocation.Z);
 //   FVector Local((X + ChunkLocation.X) * ChunkSize,
 //                 (Y + ChunkLocation.Y) * ChunkSize,
 //                 (Z + ChunkLocation.Z) * ChunkSize);
 //   
 //   float R = this->LoadRadiu;
 //   ChunkCenter += FVector(ChunkLocation.X * ChunkSize,
 //                          ChunkLocation.Y * ChunkSize, 
 //                          ChunkLocation.Z * ChunkSize);

 //   FVector LocationKEY(X + ChunkLocation.X, Y + ChunkLocation.Y, Z + ChunkLocation.Z);
	//float DistSquared = FVector::DistSquared(Local, this->PlayerLocation);
	//float Alpha = DistSquared / R;
 //   if (Alpha >= 1.f)
 //   {
 //       return false;
 //   }
 //   int32 NewPaddingSize = PaddingSize;

 //   if (Alpha > 0.2f && Alpha < 0.5f)
 //   {
 //       NewPaddingSize = PaddingSize / 2;
 //   }
 //   else if (Alpha > 0.5f)
 //   {
 //       NewPaddingSize = PaddingSize / 4;
 //   }


 //   UCxGamiProduralMeshComponent* MeshComponent = nullptr;
 //   if (this->ChunkBox.Contains(LocationKEY))
	//{
 //       const auto it = this->ChunkBox.Find(LocationKEY);
 //       auto MeshComponentWeakPointer = it->Key;

 //       bool IsValid = it->Key.IsValid();

 //       if (!IsValid && it->Value == NewPaddingSize)
 //       {
 //           return false;
 //       }
 //       else
 //       {
 //           MeshComponent = (UCxGamiProduralMeshComponent *)it->Key.Get();
 //       }

 //       if (IsValid && MeshComponent && MeshComponent->PlanetChunk->UNPADDED_CHUNK_SIZE == NewPaddingSize)
 //       {
 //           return false;
 //       }

	//}

 //   TUniquePtr<FPlanetChunk> NewChunk = MakeUnique<FPlanetChunk>(ChunkCenter, 0, ChunkSize);
 //   NewChunk->UNPADDED_CHUNK_SIZE = NewPaddingSize;
 //   NewChunk->PADDED_CHUNK_SIZE = NewPaddingSize + 2;

 //   this->ChunkBox.Add(LocationKEY,
 //       TPair<TWeakObjectPtr<UProceduralMeshComponent>, int32>(TWeakObjectPtr<UProceduralMeshComponent>(), NewPaddingSize));

 //   bool bMeshGenerated = NewChunk->GenerateMesh(NoiseGenerator);
 //   if (bMeshGenerated && NewChunk->Vertices.Num() > 0 && NewChunk->Triangles.Num() > 0 )
 //   {        

 //       if (!MeshComponent)
 //            MeshComponent = (UCxGamiProduralMeshComponent*)CreateMeshComponent();

 //       MeshComponent->Location.X = X + ChunkLocation.X;
 //       MeshComponent->Location.Y = Y + ChunkLocation.Y;
 //       MeshComponent->Location.Z = Z + ChunkLocation.Z;
 //       MeshComponent->PlanetActor = this;
 //       auto item = this->ChunkBox.Find(LocationKEY);
 //       item->Key = MeshComponent;
 //       item->Value = true;
 //       
 //       TArray<FColor> VertexColors;
 //       TArray<FProcMeshTangent> Tangents;


 //       bool Collision = this->bEnableCollision;
 //       if (NewPaddingSize != PaddingSize)
 //       {
 //           Collision = false;
 //       }

 //       MeshComponent->CreateMeshSection(
 //           0,
 //           NewChunk->Vertices,
 //           NewChunk->Triangles,
 //           NewChunk->Normals,
 //           NewChunk->UVs,
 //           VertexColors,
 //           Tangents,
 //           Collision
 //       );
 //       if (PlanetMaterial)
 //       {
 //           MeshComponent->SetMaterial(0, PlanetMaterial);
 //       }
 //       MeshComponent->PlanetChunk = MoveTemp(NewChunk);
 //   }

 //   return bMeshGenerated;
}


void APlanetActor::CircleChunk(FVector Location)
{
    auto it = this->ChunkBox.Find(Location);

    if (it != nullptr)
    {
        if (it->Key.IsValid())
        {
            it->Key->DestroyComponent();
            this->ChunkBox.Remove(Location);
        }
    }
}