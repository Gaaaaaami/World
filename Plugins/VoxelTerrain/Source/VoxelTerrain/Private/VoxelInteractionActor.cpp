// Copyright 2026 GlimmerWorks Studio. All Rights Reserved.

#include "VoxelInteractionActor.h"
#include "VoxelTerrainComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
AVoxelInteractionActor::AVoxelInteractionActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AVoxelInteractionActor::BeginPlay()
{
    Super::BeginPlay();
    CachedPC = UGameplayStatics::GetPlayerController(this, 0);
}

void AVoxelInteractionActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!CachedPC) return;

    const FKey LeftMouseKey(TEXT("LeftMouseButton"));
    const FKey RightMouseKey(TEXT("RightMouseButton"));

    const bool bLeftDown = CachedPC->IsInputKeyDown(LeftMouseKey);
    const bool bRightDown = CachedPC->IsInputKeyDown(RightMouseKey);

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    if (bLeftDown && (CurrentTime - LastLeftClickTime > EditCooldown))
    {
        HandleMouseClick(false);
        LastLeftClickTime = CurrentTime;
    }
    else if (bRightDown && (CurrentTime - LastRightClickTime > EditCooldown))
    {
        HandleMouseClick(true);
        LastRightClickTime = CurrentTime;
    }
}

void AVoxelInteractionActor::HandleMouseClick(bool bIsAdd)
{
    if (!CachedPC) return;

    FVector CamLoc;
    FRotator CamRot;
    CachedPC->GetPlayerViewPoint(CamLoc, CamRot);

    const FVector End = CamLoc + CamRot.Vector() * MaxTraceDistance;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(CachedPC->GetPawn());

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params);

    if (bHit)
    {
        if (AActor* HitActor = Hit.GetActor())
        {
            if (UVoxelTerrainComponent* VoxelComp = HitActor->FindComponentByClass<UVoxelTerrainComponent>())
            {
                // ⭐ 偏置自适应：小半径也能穿透旧表面，避免"点了没反应"
                const float OffsetDist = FMath::Max(EditRadius * 0.5f, 50.f);
                FVector EditLocation = Hit.Location + CamRot.Vector() * OffsetDist;

                VoxelComp->ApplyEdit(EditLocation, EditRadius, bIsAdd);
            }
        }
    }

}