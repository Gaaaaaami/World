#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "PlanetChunk.h"  // Include the complete definition
#include "PlanetActor.generated.h"

class UNoiseGenerator;

UCLASS(BlueprintType, Blueprintable)
class WORLD_API APlanetActor : public AActor
{
    GENERATED_BODY()
    
public:    
    APlanetActor();

protected:
    virtual void BeginPlay() override;

public:
    
    /** Size of each chunk in world units */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float ChunkSize = 64.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float LoadRadiu = 64.0f;
    /** Number of chunks per axis (creates ChunksPerAxis^3 total chunks) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    int32 ChunksPerAxis = 16;
    
    /** Number of voxels per chunk (base resolution) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    int32 VoxelsPerChunk = 16;
    
    /** Enable collision for generated meshes */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    bool bEnableCollision = false;
    
    /** Material to apply to planet surface */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
    UMaterialInterface* PlanetMaterial = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	FVector PlayerLocation;
    /** Noise generator for terrain */
	UPROPERTY()
    TObjectPtr<UNoiseGenerator> NoiseGenerator = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
	TSubclassOf<UNoiseGenerator> NoiseGeneratorClass;

    /** Initialize or reinitialize the planet with current parameters */
    UFUNCTION(BlueprintCallable, Category = "Planet")
    void InitializePlanet();

    UFUNCTION(BlueprintCallable, Category = "Circle")
    void CircleChunk(FVector Location);
public:

    UPROPERTY()
    TArray<TWeakObjectPtr<UProceduralMeshComponent>> MeshComponents;
    
    UProceduralMeshComponent* CreateMeshComponent();
    
    void GenerateAllChunks( UProceduralMeshComponent* component = nullptr, FPlanetChunk* planet_chunk = nullptr);
    
    bool GenerateChunk(int32 X, int32 Y, int32 Z, FVector ChunkCenter, UProceduralMeshComponent *component = nullptr, FPlanetChunk* planet_chunk = nullptr);
public:
    UPROPERTY()
	TMap<FVector, TWeakObjectPtr<UProceduralMeshComponent>> ChunkBox;
public:

    TArray<TArray<uint8>> NoiseTables;
};