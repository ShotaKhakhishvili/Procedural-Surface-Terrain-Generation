// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"

#include "MeshFunctionLibrary.h"
#include "../Structures/MeshData.h"
#include "ChunkFunctionLibrary.generated.h"


UCLASS(BlueprintType)
class UChunkFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
private:
    static float                m_noiseScale;
    static float                m_heightMultiplier;
    static float                m_chunkWidth;
    static float                m_UVScale;
    static uint8                m_maxLOD;

public:

    UChunkFunctionLibrary()
    {
    }

    UFUNCTION(BlueprintCallable)
    static void SetTerrainGenerationSettings(
        const float         chunkWidth,
        const float         noiseScale,
        const float         heightMultiplier,
        const float         UVScale,
        const uint8         maxLOD
    )
    {
        m_heightMultiplier = heightMultiplier;
        m_noiseScale = noiseScale;
        m_chunkWidth = chunkWidth;
        m_UVScale = UVScale;
        m_maxLOD = maxLOD;
    }

    static FORCEINLINE float GetChunkWidth()        { return m_chunkWidth;          }
    static FORCEINLINE float GetNoiseScale()        { return m_noiseScale;          }
    static FORCEINLINE float GetHeightMultiplier()  { return m_heightMultiplier;    }
    static FORCEINLINE float GetUVScale()           { return m_UVScale;             }
    static FORCEINLINE uint8 GetMaxLOD()            { return m_maxLOD;              }

    static FMeshData& GetChunkData_Border_Up     (const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale);
    static FMeshData& GetChunkData_Border_Down   (const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale);
    static FMeshData& GetChunkData_Border_Left   (const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale);
    static FMeshData& GetChunkData_Border_Right  (const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale);

    static TArray<int32>& GetTopLod_Center_Vertices(
        const FVector2D&            Pos
    );

    static TArray<int32>& GetLod_Additionals_Vertices
    (
        const TArray<int32>&        topLodVertices,
        const FVector2D             Pos,
        const uint8                 LOD
    );

    static FMeshData& GetChunkData_Center(
        const TArray<int32>&        wholeChunk_additionals,
        const FVector2D             Pos,
        const int8                  LOD
    );

    static FMeshData& GetChunkData_Border(
        const FMeshData&            wholeChunk_additionals,
        const FChunkPartSelector&   chunkPartSelector
    );

    static FChunkLodData& GenerateChunkData_LOD(
        const FVector2D&            Pos,
        const uint8                 LOD
    );
};