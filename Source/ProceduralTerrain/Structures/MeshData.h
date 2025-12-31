// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ProceduralMeshComponent.h"
#include "MeshData.generated.h"

// Structure to hold mesh data
USTRUCT(BlueprintType)
struct FMeshData
{
    GENERATED_BODY()

public:

	// Raw mesh data arrays
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVector>             vertices;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32>               triangles;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVector2D>           UVs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVector>             normals;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FProcMeshTangent>    tangents;

    FMeshData() {}

	// Copy constructor
    FMeshData(const FMeshData& meshData)
        : vertices(meshData.vertices)
        , triangles(meshData.triangles)
        , UVs(meshData.UVs)
        , normals(meshData.normals)
        , tangents(meshData.tangents)
    {
    }

	// Move constructor
    FMeshData(FMeshData&& Other) noexcept
        : vertices(MoveTemp(Other.vertices))
        , triangles(MoveTemp(Other.triangles))
        , UVs(MoveTemp(Other.UVs))
        , normals(MoveTemp(Other.normals))
        , tangents(MoveTemp(Other.tangents))
    {
    }

	// Pre-allocate arrays based on expected vertex count
    FMeshData(const FVector2D vertexCount, const bool includeNormalsAndTangents = false)
    {
        vertices.Reserve(vertexCount.X * vertexCount.Y);
        UVs.Reserve(vertexCount.X * vertexCount.Y);
        triangles.Reserve((vertexCount.X - 1) * (vertexCount.Y - 1) * 6);

        if (includeNormalsAndTangents)
        {
            normals.Reserve(vertexCount.X * vertexCount.Y);
            tangents.Reserve(vertexCount.X * vertexCount.Y);
        }
    }

    // Copy assignment
    FMeshData& operator=(const FMeshData& Other)
    {
        if (this != &Other)
        {
            vertices = Other.vertices;
            triangles = Other.triangles;
            UVs = Other.UVs;
            normals = Other.normals;
            tangents = Other.tangents;
        }
        return *this;
    }

	// Move assignment
    FMeshData& operator=(FMeshData&& Other) noexcept
    {
        if (this != &Other)
        {
            vertices = MoveTemp(Other.vertices);
            triangles = MoveTemp(Other.triangles);
            UVs = MoveTemp(Other.UVs);
            normals = MoveTemp(Other.normals);
            tangents = MoveTemp(Other.tangents);
        }
        return *this;
    }
};


// Directions for chunk borders
UENUM(BlueprintType)
enum class Direction : uint8 {
    Left    = 0, 
    Right   = 1, 
    Up      = 2, 
    Down    = 3,
    Center  = 4
};

// Structure to hold LOD information for a chunk, including downscale masks for borders
USTRUCT(BlueprintType)
struct FChunkLodInfos
{
    GENERATED_BODY()

public:
	// Bitmask constants for directions
    static constexpr uint8 mask_left    = (1u << (static_cast<uint8>(Direction::Left)));
    static constexpr uint8 mask_right   = (1u << (static_cast<uint8>(Direction::Right)));
    static constexpr uint8 mask_up      = (1u << (static_cast<uint8>(Direction::Up)));
    static constexpr uint8 mask_down    = (1u << (static_cast<uint8>(Direction::Down)));

	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8           LOD;                    // Level of Detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8           downscales_masked;      // Bitmask for downscaled borders

    FChunkLodInfos()                                                            :   LOD(0),     downscales_masked(0)            {}
    FChunkLodInfos(uint8 LODin, uint8 downscalesIn)                             :   LOD(LODin), downscales_masked(downscalesIn) {}
    FChunkLodInfos(uint8 LODin, bool left, bool right, bool up, bool down)      :   LOD(LODin), downscales_masked(0)
    {
        if (left)
            downscales_masked = mask_left;
        if (right)
            downscales_masked |= mask_right;
        if (up)
            downscales_masked |= mask_up;
        if (down)
            downscales_masked |= mask_down;
    }

    FORCEINLINE bool GetDownscale(Direction direction)const
    {
        const int8 mask = (1u << static_cast<int>(direction));
        return (downscales_masked & mask) != 0;
    }
};

// Structure to manage which LODs are active for a chunk
USTRUCT(BlueprintType)
struct FChunkLodSelector
{
    GENERATED_BODY()

private:
    TArray<bool>    m_LODs;
    uint8           m_activeLODs = 0;
public:

    FORCEINLINE uint8 GetActiveLODs()const { return m_activeLODs; }

    FORCEINLINE bool GetLOD(uint8 LOD)const
    {
        check(m_LODs.IsValidIndex(LOD));
        return m_LODs[LOD];
    }

	// Initialize the LOD selector with a given number of LODs
    FORCEINLINE FChunkLodSelector& InitializeNum(uint8 size)
    {
        m_LODs.SetNum(size+1);
        m_activeLODs = 0;
        return *this;
    }

    FORCEINLINE FChunkLodSelector& SetLOD(uint8 indx, bool newValue)
    {
		check(m_LODs.IsValidIndex(indx));

        if (m_LODs[indx] != newValue)
        {
            if (newValue) { ++m_activeLODs; }
            else { --m_activeLODs; }
            m_LODs[indx] = newValue;
        }

        return *this;
    }
};

// Structure to hold mesh data for a specific LOD of a chunk
USTRUCT(BlueprintType)
struct FChunkLodData
{
    GENERATED_BODY()

public:
    FMeshData           Center;
    FMeshData           borders_normal[4];
    FMeshData           borders_downscaled[4];

    FChunkLodData() = default;
    FChunkLodData(const FChunkLodData& Other) = default;
    FChunkLodData(FChunkLodData&& Other) noexcept = default;
    FChunkLodData& operator=(const FChunkLodData& Other) = default;
    FChunkLodData& operator=(FChunkLodData&& Other) noexcept = default;
};

// Structure to manage multiple LODs for a chunk
USTRUCT(BlueprintType)
struct FChunkData
{
    GENERATED_BODY()

private:
    TArray<FChunkLodData>   m_LODs;
    uint32                  m_LODMask = 0;
public:

    inline bool ContainsLOD(uint8 index) const 
    { 
		check(index < 32);
        return (m_LODMask & (1u << index)) != 0; 
    }

    FORCEINLINE FChunkLodData& GetLOD(uint8 index) 
    {
		check(m_LODs.IsValidIndex(index));
        return m_LODs[index]; 
    }

    void Reset()
    {
        m_LODs.Reset();
        m_LODMask = 0;
    }

	// Initialize the chunk data with a maximum LOD
    void Initialize(uint8 maxLOD)
    {
		check(maxLOD < 32);
        m_LODs.SetNum(maxLOD + 1);
        m_LODMask = 0;
    }

    FORCEINLINE void RemoveLOD(uint8 index)
    {
		check(m_LODs.IsValidIndex(index));
        m_LODs[index] = FChunkLodData{};
        m_LODMask &= (~(1u << index));
    }

    FORCEINLINE void AddNewLOD(uint8 index, FChunkLodData&& data)
    {
		check(m_LODs.IsValidIndex(index));

        m_LODs[index] = MoveTemp(data);
        m_LODMask |= (1u << index);
    }

    FORCEINLINE int32 GetMaxLowerLOD(int32 maxLOD) const
    {
		maxLOD = FMath::Min<int32>(maxLOD, m_LODs.Num() - 1);

        for (int32 i = maxLOD; i > 0; i--)
        {
            if ((1u << i) & m_LODMask)
            {
                return i;
            }
        }
        return 0;
    }

    FORCEINLINE int32 GetMinHigherLOD(int32 maxLOD) const
    {
		maxLOD = FMath::Max<int32>(maxLOD, 0);

        for (int32 i = maxLOD; i < m_LODs.Num(); i++)
        {
            if ((1u << i) & m_LODMask)
            {
                return i;
            }
        }
        return 0;
    }

};

// Structure to select a specific part of a chunk based on LOD and border direction
USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct FChunkPartSelector
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool            downscaled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) Direction       borderDirection;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8           LOD;

    FChunkPartSelector() 
        :   downscaled(false),
            borderDirection(Direction::Center),
            LOD(0)
    {}
    FChunkPartSelector(const uint8 LODIn, const Direction borderDirectionIn, const bool downscaledIn = false)
        :   downscaled(downscaledIn),
            borderDirection(borderDirectionIn),
            LOD(LODIn)
    {}

    FORCEINLINE bool IsBorder()const
    {
        return borderDirection != Direction::Center;
    }

};

USTRUCT(BlueprintType)
struct FArrayUint8
{
    GENERATED_BODY()
public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<uint8> array;

    FArrayUint8() {}
    FArrayUint8(uint32 size)
    {
        array.SetNum(size);
    }
};

UCLASS(BlueprintType)
class UMeshDataWrapper : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    FMeshData MeshData;

    UFUNCTION(BlueprintCallable)
    static UMeshDataWrapper* Create(const FMeshData& InData)
    {
        UMeshDataWrapper* Obj = NewObject<UMeshDataWrapper>();
        Obj->MeshData = InData;
        return Obj;
    }
};


