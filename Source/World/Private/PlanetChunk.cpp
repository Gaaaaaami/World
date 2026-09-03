#include "PlanetChunk.h"
#include "NoiseGenerator.h"
#include "SurfaceNets.h"
#include "SurfaceNetsUE.h"

FPlanetChunk::FPlanetChunk()
    : Position(FVector::ZeroVector)
    , LODLevel(0)
    , Size(100)
    , VoxelSize(100.f)
    , bIsGenerated(false)
    , bIsGenerating(false)
    , bIsEmpty(false)
    , DistanceFromCamera(0.0f)
{
}

FPlanetChunk::FPlanetChunk(const FVector& InPosition, int32 InLODLevel, FVector InSize, float InVoxelSize)
    : Position(InPosition)
    , LODLevel(InLODLevel)
    , Size(InSize)
    , VoxelSize(InVoxelSize)
    , bIsGenerated(false)
    , bIsGenerating(false)
    , bIsEmpty(false)
    , DistanceFromCamera(0.0f)
{
}

bool FPlanetChunk::GenerateMesh(const UNoiseGenerator* NoiseGenerator)
{
    if (!NoiseGenerator || bIsGenerating)
    {
        return false;
    }

    bIsGenerating = true;
    ClearMesh();

    int32 PaddedSize = 10.f;
    FVector PaddedOrigin;

    // Generate density field with padding (like Rust implementation)
    if (!GeneratePaddedDensityField(NoiseGenerator, DensityField, PaddedSize, PaddedOrigin, VoxelSize))
    {
        bIsGenerating = false;
        bIsEmpty = true;
        bIsGenerated = true;
        //UE_LOG(LogSurfaceNets, Warning, TEXT("Failed to generate density field for chunk at %s"), *Position.ToString());
        return false;
    }
#if 0
    // Early exit if no surface (like Rust optimization)
    if (!FSurfaceNets::HasSurfaceInChunk(DensityField))
    {
        bIsGenerating = false;
        bIsEmpty = true;
        bIsGenerated = true;
        //UE_LOG(LogSurfaceNets, Verbose, TEXT("Chunk at %s has no surface"), *Position.ToString());
        return false;
    }
#endif

    // Generate mesh using Surface Nets with Rust-like bounds
    FSurfaceNets SurfaceNets;
    SurfaceNets.GenerateMesh(
        DensityField,
        PaddedSize,
        VoxelSize,
        PaddedOrigin,
        Vertices,
        Triangles,
        Normals,
        FIntVector(0, 0, 0),                    // Min bounds
        FIntVector(Size.X, Size.Y, Size.Z)     // Max bounds (17,17,17) like Rust [0;3], [17;3]
    );

    // Generate UVs
    UVs.SetNum(Vertices.Num());
    for (int32 i = 0; i < Vertices.Num(); i++)
    {
        // Simple planar UV mapping
        FVector LocalPos = Vertices[i] - Position;
        UVs[i] = FVector2D(
            (LocalPos.X / VoxelSize) + 0.5f,
            (LocalPos.Y / VoxelSize) + 0.5f
        );
    }

    bIsGenerating = false;
    bIsGenerated = true;
    bIsEmpty = (Vertices.Num() == 0);

    //UE_LOG(LogSurfaceNets, Verbose, TEXT("Generated chunk at %s with %d vertices, %d triangles"), 
           //*Position.ToString(), Vertices.Num(), Triangles.Num() / 3);

    return !bIsEmpty;
}

void FPlanetChunk::ClearMesh()
{
    Vertices.Empty();
    Triangles.Empty();
    Normals.Empty();
    UVs.Empty();
    bIsGenerated = false;
    bIsEmpty = false;
}

int32 FPlanetChunk::GetVoxelResolution() const
{
    // LOD-based resolution like original design
    return FMath::Max(8, UNPADDED_CHUNK_SIZE >> LODLevel);
}

bool FPlanetChunk::GeneratePaddedDensityField(
    const UNoiseGenerator* NoiseGenerator,
    TArray<float>& OutDensityField,
    int32& OutPaddedSize,
    FVector& OutPaddedOrigin,
    float& OutVoxelSize)
{
    if (!NoiseGenerator)
    {
        return false;
    }

    //OutPaddedSize = PADDED_CHUNK_SIZE;
    //OutVoxelSize = Size / UNPADDED_CHUNK_SIZE;
    OutPaddedOrigin = Position - (this->Size / 2.f);//FVector(OutVoxelSize / 2.f); //- FVector(Size * 0.5f) - FVector(OutVoxelSize);


    if (OutDensityField.IsEmpty())
    {
		OutDensityField.SetNum(this->Size.X * this->Size.Y * this->Size.Z);
        for (int32 x = 0; x < this->Size.X; x++)
        {
            for (int32 y = 0; y < this->Size.Y; y++)
            {
                for (int32 z = 0; z < this->Size.Z; z++)
                {
                    float Density = NoiseGenerator->SampleDensity(FVector(x,y,z));
                    int32 Index = x + y * this->Size.X + z * this->Size.X * this->Size.Y;
                    if (OutDensityField.IsValidIndex(Index) || true)
                    {
                        OutDensityField[Index] = Density;

                        if (Density > 0.0f)
                        {
                            HasPositive = true;
                        }
                        else
                        {
                            HasNegativeOrZero = true;
                        }

                        if (HasPositive && HasNegativeOrZero)
                        {
                            HasSurface = true;
                        }
                    }
                }
            }
        }

    }

    return HasSurface;
}