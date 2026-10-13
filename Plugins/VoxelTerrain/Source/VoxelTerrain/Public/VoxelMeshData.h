#pragma once
#include "CoreMinimal.h"

struct FVoxelMeshData
{
    TArray<FVector3f> Positions;
    TArray<FVector3f> Normals;
    TArray<FColor>    Colors;        // ¡û ÐÂÔö
    TArray<uint32>    Indices;

    void Reset()
    {
        Positions.Reset();
        Normals.Reset();
        Colors.Reset();
        Indices.Reset();
    }

    int32 NumTriangles() const { return Indices.Num() / 3; }
};