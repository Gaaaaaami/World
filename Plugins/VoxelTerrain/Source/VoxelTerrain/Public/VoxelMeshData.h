#pragma once
#include "CoreMinimal.h"

struct FVoxelMeshData
{
    TArray<FVector3f> Positions;
    TArray<FVector3f> Normals;
    TArray<FVector2f> UVs;
    //TArray<FColor>    Colors;
    TArray<uint32>    Indices;
    TArray<float> SurfaceHeights;  
    // ⭐ Async 线程预算好的 world-space bounds，主线程只做 O(1) 合并
    FBox              Bounds = FBox(ForceInit);

    void Reset()
    {
        Positions.Reset();
        Normals.Reset();
        UVs.Reset();
        //Colors.Reset();
        Indices.Reset();
        SurfaceHeights.Reset();
        Bounds.Init();
    }

    int32 NumTriangles() const { return Indices.Num() / 3; }
};