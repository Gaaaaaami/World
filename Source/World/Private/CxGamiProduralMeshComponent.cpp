// Fill out your copyright notice in the Description page of Project Settings.


#include "CxGamiProduralMeshComponent.h"
#include <cmath>

UCxGamiProduralMeshComponent::UCxGamiProduralMeshComponent(const FObjectInitializer& ObjectInitializer):UProceduralMeshComponent(ObjectInitializer)
{

}

UCxGamiProduralMeshComponent::~UCxGamiProduralMeshComponent()
{

}

void UCxGamiProduralMeshComponent::BindPlanetChunk(FPlanetChunk *InNewChunk)
{
	this->NewChunk = InNewChunk;
}

void UCxGamiProduralMeshComponent::BindPlanetActor(APlanetActor* InPlanetActor)
{
	this->PlanetActor = InPlanetActor;
}
#if 0
void UCxGamiProduralMeshComponent::Destruction(FVector InLocation, float BrushRadiusCm)
{
	FVector VoxelPosition = InLocation / this->PlanetActor->ChunkSize;
	VoxelPosition.X = std::floor(VoxelPosition.X);
	VoxelPosition.Y = std::floor(VoxelPosition.Y);
	VoxelPosition.Z = std::floor(VoxelPosition.Z);

	float SubVoxelSize = this->PlanetActor->ChunkSize / float(FPlanetChunk::UNPADDED_CHUNK_SIZE);
	FVector SubVoxelPosition = InLocation / SubVoxelSize;
	SubVoxelPosition.X = std::floor(SubVoxelPosition.X);
	SubVoxelPosition.Y = std::floor(SubVoxelPosition.Y);
	SubVoxelPosition.Z = std::floor(SubVoxelPosition.Z);
	VoxelPosition.X = FMath::Clamp(VoxelPosition.X, 0.f, static_cast<float>(MAX_int32));
	VoxelPosition.Y = FMath::Clamp(VoxelPosition.Y, 0.f, static_cast<float>(MAX_int32));
	VoxelPosition.Z = FMath::Clamp(VoxelPosition.Z, 0.f, static_cast<float>(MAX_int32));
	SubVoxelPosition.X = FMath::Clamp(SubVoxelPosition.X, 0.f, static_cast<float>(MAX_int32)) - (VoxelPosition.X * FPlanetChunk::UNPADDED_CHUNK_SIZE);
	SubVoxelPosition.Y = FMath::Clamp(SubVoxelPosition.Y, 0.f, static_cast<float>(MAX_int32)) - (VoxelPosition.Y * FPlanetChunk::UNPADDED_CHUNK_SIZE);
	SubVoxelPosition.Z = FMath::Clamp(SubVoxelPosition.Z, 0.f, static_cast<float>(MAX_int32));

	int32 OutPaddedSize = FPlanetChunk::PADDED_CHUNK_SIZE;
	if (this->NewChunk)
	{
		
		TSet<TObjectPtr<UCxGamiProduralMeshComponent>> update_chunk;

		float BrushRadiusCm = 6.0f;  // 你自己的半径
		float DigStrength = 0.5f;
		int Limit = std::ceil(BrushRadiusCm / SubVoxelSize);

		for (int x = -Limit; x <= Limit; x++)
		{
			for (int y = -Limit; y <= Limit; y++)
			{
				for (int z = -Limit; z <= Limit; z++)
				{
		
					int step = Limit - (-Limit);
					int RelativeX = (SubVoxelPosition.X + x + (step / 2));
					int RelativeY = (SubVoxelPosition.Y + y + (step / 2));
					int RelativeZ = (SubVoxelPosition.Z + z);
					int WorldX = RelativeX * SubVoxelSize + (VoxelPosition.X * FPlanetChunk::PADDED_CHUNK_SIZE);
					int WorldY = RelativeY * SubVoxelSize + (VoxelPosition.Y * FPlanetChunk::PADDED_CHUNK_SIZE);
					int WorldZ = RelativeZ * SubVoxelSize + (VoxelPosition.Z * FPlanetChunk::PADDED_CHUNK_SIZE);
					float D = FVector::DistSquared(FVector(WorldX, WorldY, WorldZ), InLocation);
					float P = D / BrushRadiusCm;
					P = 1.f - P;
					P = z - P;

					int32 idx = RelativeX + RelativeY * OutPaddedSize + RelativeZ * OutPaddedSize * OutPaddedSize;
					if (this->NewChunk->DensityField.IsValidIndex(idx))
					{
						this->NewChunk->DensityField[idx] = FMath::Max(this->NewChunk->DensityField[idx], P);
						int32 XEdge = std::floor(float(WorldX) / this->PlanetActor->ChunkSize);
						if (XEdge > Location.X)
						{
							int32 right_chunk_cell_x = std::abs(FPlanetChunk::PADDED_CHUNK_SIZE - WorldX);
							int32 right_chunk_cell_idx = right_chunk_cell_x + RelativeY * OutPaddedSize + RelativeZ * OutPaddedSize * OutPaddedSize;
							UE_LOG(LogTemp, Log, TEXT("XEdge: %d|right_chunk_cell_x:%d"), XEdge, right_chunk_cell_x);

						}
					}
					else
					{

					}

					DrawDebugSphere(GetWorld(), FVector(WorldX, WorldY, WorldZ), 0.5, 1.f, FColor::Green, true);

				}

			}
		
		}
		int step = Limit - (-Limit);
		//DrawDebugSphere(GetWorld(), InLocation, 12, 32.f, FColor::Red, true);


		for (auto it : update_chunk)
		{
			it->PlanetActor->GenerateAllChunks(it, it->NewChunk);
		}
		this->PlanetActor->GenerateAllChunks(this, this->NewChunk);
	}


}
#else
void UCxGamiProduralMeshComponent::Destruction(FVector InLocation, float BrushRadiusCm, float Strength, bool UpdateChunkCell)
{
	FVector VoxelPosition = InLocation / this->PlanetActor->ChunkSize;
	VoxelPosition.X = std::floor(VoxelPosition.X);
	VoxelPosition.Y = std::floor(VoxelPosition.Y);
	VoxelPosition.Z = std::floor(VoxelPosition.Z);

	float SubVoxelSize = this->PlanetActor->ChunkSize / float(FPlanetChunk::UNPADDED_CHUNK_SIZE);
	FVector SubVoxelPosition = InLocation / SubVoxelSize;
	SubVoxelPosition.X = std::floor(SubVoxelPosition.X);
	SubVoxelPosition.Y = std::floor(SubVoxelPosition.Y);
	SubVoxelPosition.Z = std::floor(SubVoxelPosition.Z);
	VoxelPosition.X = FMath::Clamp(VoxelPosition.X, 0.f, static_cast<float>(MAX_int32));
	VoxelPosition.Y = FMath::Clamp(VoxelPosition.Y, 0.f, static_cast<float>(MAX_int32));
	VoxelPosition.Z = FMath::Clamp(VoxelPosition.Z, 0.f, static_cast<float>(MAX_int32));
	SubVoxelPosition.X = FMath::Clamp(SubVoxelPosition.X, 0.f, static_cast<float>(MAX_int32)) - (VoxelPosition.X * FPlanetChunk::UNPADDED_CHUNK_SIZE);
	SubVoxelPosition.Y = FMath::Clamp(SubVoxelPosition.Y, 0.f, static_cast<float>(MAX_int32)) - (VoxelPosition.Y * FPlanetChunk::UNPADDED_CHUNK_SIZE);
	SubVoxelPosition.Z = FMath::Clamp(SubVoxelPosition.Z, 0.f, static_cast<float>(MAX_int32)) - (VoxelPosition.Y * FPlanetChunk::UNPADDED_CHUNK_SIZE);
	int32 OutPaddedSize = FPlanetChunk::PADDED_CHUNK_SIZE;
	
	if (this->NewChunk != nullptr)
	{

		float S = std::sqrt(BrushRadiusCm);
		float S_brushRadiuCm = BrushRadiusCm * BrushRadiusCm;
		for (int32 z = 0; z < OutPaddedSize; z++)
		{
			for (int32 y = 0; y < OutPaddedSize; y++)
			{
				for (int32 x = 0; x < OutPaddedSize; x++)
				{
					float WorldX = x * SubVoxelSize + (Location.X * PlanetActor->ChunkSize);
					float WorldY = y * SubVoxelSize + (Location.Y * PlanetActor->ChunkSize);
					float WorldZ = z * SubVoxelSize + (Location.Z * PlanetActor->ChunkSize);

					float D = FVector::DistSquared(FVector(WorldX - S, WorldY - S, WorldZ - S), InLocation);//FVector::Dist(FVector(WorldX - S, WorldY - S, WorldZ), InLocation);//FVector::DistSquared(FVector(WorldX - S, WorldY - S, WorldZ), InLocation);
					float P = D / S_brushRadiuCm;

					int32 idx = x + y * OutPaddedSize + z * OutPaddedSize * OutPaddedSize;
					if (this->NewChunk->DensityField.IsValidIndex(idx))
					{
						if (P < 1.f)
							this->NewChunk->DensityField[idx] += FMath::Max(this->NewChunk->DensityField[idx], (1.0 - P) * Strength);
					}
				}
			}
		}

		this->PlanetActor->GenerateAllChunks(this, this->NewChunk);


		for (int32 x = -1; x <= 1; x++)
		{
			for (int32 y = -1; y <= 1; y++)
			{
				for (int32 z = -1; z <= 1; z++)
				{

					if (x == 0 && y == 0 && z == 0)
					{
						continue;
					}

					FVector ChunkLocation = Location + FVector(x, y, z);
					this->UpdateChunk(ChunkLocation, InLocation, BrushRadiusCm, Strength, UpdateChunkCell);
				}
			}
		}
	}
}

bool UCxGamiProduralMeshComponent::UpdateChunk(FVector ChunkLocation, FVector InLocation, float BrushRadiusCm, float Strength, bool UpdateChunkCell)
{
	int idx = ChunkLocation.Z * (this->PlanetActor->ChunksPerAxis * this->PlanetActor->ChunksPerAxis) + ChunkLocation.Y * this->PlanetActor->ChunksPerAxis + ChunkLocation.X;
	if (this->PlanetActor->MeshComponents.IsValidIndex(idx))
	{
		if (this->PlanetActor->MeshComponents[idx].IsValid())
		{
			UCxGamiProduralMeshComponent* component = Cast<UCxGamiProduralMeshComponent>(this->PlanetActor->MeshComponents[idx].Get());
			if (!UpdateChunkCell)
			{
				component->Destruction(InLocation, BrushRadiusCm, Strength, true);
				return true;
			}
		}
	}
	return false;
}

#endif


