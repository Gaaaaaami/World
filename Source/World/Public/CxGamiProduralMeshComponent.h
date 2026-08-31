
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PlanetChunk.h"
#include "PlanetActor.h"
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
	TWeakObjectPtr<APlanetActor> PlanetActor;
	TUniquePtr<FPlanetChunk> PlanetChunk;

	FVector Location;
};
