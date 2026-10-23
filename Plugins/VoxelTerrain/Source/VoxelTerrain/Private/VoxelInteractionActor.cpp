#include "VoxelInteractionActor.h"
#include "VoxelTerrainComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

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
                // ⭐ 核心修复：如果打中了旧网格，顺着视线强制往里面挖 50 单位，确保能穿透到下一层
                FVector EditLocation = Hit.Location + CamRot.Vector() * 50.f;

                VoxelComp->ApplyEdit(EditLocation, EditRadius, bIsAdd);

                DrawDebugLine(GetWorld(), CamLoc, Hit.Location, FColor::Green, false, 1.0f, 0, 2.0f);
                DrawDebugSphere(GetWorld(), EditLocation, EditRadius, 12,
                    bIsAdd ? FColor::Green : FColor::Red, false, 1.0f);
            }
        }
    }
    else
    {
        DrawDebugLine(GetWorld(), CamLoc, End, FColor::Red, false, 1.0f, 0, 2.0f);
    }
}