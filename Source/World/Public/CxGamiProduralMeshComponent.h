
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CxGamiProduralMeshComponent.generated.h"

/**
 * 
 */

UCLASS()
class WORLD_API UCxGamiProduralMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
public:
	UCxGamiProduralMeshComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UCxGamiProduralMeshComponent();
public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
public:
	FVector Location;
	AActor* DynamicLandScape = nullptr;
	int32 LOD = 0;

};
