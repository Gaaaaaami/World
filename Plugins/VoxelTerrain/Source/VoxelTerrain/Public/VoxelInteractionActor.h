#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelInteractionActor.generated.h"

class UVoxelTerrainComponent;

UCLASS()
class VOXELTERRAIN_API AVoxelInteractionActor : public AActor
{
    GENERATED_BODY()

public:
    AVoxelInteractionActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Voxel Interaction")
    float MaxTraceDistance = 5000.f;

    UPROPERTY(EditAnywhere, Category = "Voxel Interaction")
    float EditRadius = 400.f;

    UPROPERTY(EditAnywhere, Category = "Voxel Interaction")
    float EditCooldown = 0.2f;

private:
    void HandleMouseClick(bool bIsAdd);

    TObjectPtr<APlayerController> CachedPC;
    float LastLeftClickTime = 0.f;
    float LastRightClickTime = 0.f;
};