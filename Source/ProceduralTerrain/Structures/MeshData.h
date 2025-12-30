// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ProceduralMeshComponent.h"
#include "MeshData.generated.h"

USTRUCT(BlueprintType)
struct FMeshData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadonly)
    TArray<FVector> vertices;

    UPROPERTY(EditAnywhere, BlueprintReadonly)
    TArray<int32> triangles;

    UPROPERTY(EditAnywhere, BlueprintReadonly)
    TArray<FVector2D> UVs;

    UPROPERTY(EditAnywhere, BlueprintReadonly)
    TArray<FVector> normals;

    UPROPERTY(EditAnywhere, BlueprintReadonly)
    TArray<FProcMeshTangent> tangents;

    FMeshData() {}

    FMeshData(const FMeshData& meshData)
        : vertices(meshData.vertices)
        , triangles(meshData.triangles)
        , UVs(meshData.UVs)
        , normals(meshData.normals)
        , tangents(meshData.tangents)
    {
    }

    FMeshData(FMeshData&& Other) noexcept
        : vertices(MoveTemp(Other.vertices))
        , triangles(MoveTemp(Other.triangles))
        , UVs(MoveTemp(Other.UVs))
        , normals(MoveTemp(Other.normals))
        , tangents(MoveTemp(Other.tangents))
    {
    }

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
};

UENUM(BlueprintType)
enum class Direction : uint8 {
    Left    = 0, 
    Right   = 1, 
    Up      = 2, 
    Down    = 3,
    Center  = 4
};

USTRUCT(BlueprintType)
struct FChunkLodInfos
{
    GENERATED_BODY()

public:
    static constexpr uint8 mask_left    = (1 << (static_cast<uint8>(Direction::Left)));
    static constexpr uint8 mask_right   = (1 << (static_cast<uint8>(Direction::Right)));
    static constexpr uint8 mask_up      = (1 << (static_cast<uint8>(Direction::Up)));
    static constexpr uint8 mask_down    = (1 << (static_cast<uint8>(Direction::Down)));

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8           LOD;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8           downscales_masked;

    FChunkLodInfos()
        :LOD(0), downscales_masked(0)
    {}

    FChunkLodInfos(uint8 LODin, bool left, bool right, bool up, bool down)
        :LOD(LODin), downscales_masked(0)
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

    FChunkLodInfos(uint8 LODin, uint8 downscalesIn)
        :LOD(LODin), downscales_masked(downscalesIn)
    {
    }

    FORCEINLINE bool GetDownscale(Direction direction)const
    {
        const int mask = (1 << (static_cast<int>(direction)));
        return downscales_masked & mask;
    }
};

USTRUCT(BlueprintType)
struct FChunkLodSelector
{
    GENERATED_BODY()

private:
    TArray<bool>    m_LODs;
    uint8           m_activeLODs;
public:
    FORCEINLINE FChunkLodSelector& InitializeNum(uint8 size)
    {
        m_LODs.SetNum(size+1);
        m_activeLODs = 0;
        return *this;
    }

    FORCEINLINE FChunkLodSelector& SetLOD(uint8 indx, bool newValue)
    {
        if (m_LODs[indx] > newValue)
            m_activeLODs--;
        else if (m_LODs[indx] < newValue)
            m_activeLODs++;

        m_LODs[indx] = newValue;

        return *this;
    }

    FORCEINLINE bool GetLOD(uint8 LOD)const
    {
        check(LOD < m_LODs.Num());
        return m_LODs[LOD];
    }

    FORCEINLINE uint8 GetActiveLODs()const 
    {
        return m_activeLODs;
    }
};

USTRUCT(BlueprintType)
struct FChunkLodData
{
    GENERATED_BODY()

public:
    FMeshData Center;
    FMeshData borders_normal[4];
    FMeshData borders_downscaled[4];

    FChunkLodData() = default;
    FChunkLodData(const FChunkLodData& Other) = default;
    FChunkLodData(FChunkLodData&& Other) noexcept = default;
    FChunkLodData& operator=(const FChunkLodData& Other) = default;
    FChunkLodData& operator=(FChunkLodData&& Other) noexcept = default;
};

USTRUCT(BlueprintType)
struct FChunkData
{
    GENERATED_BODY()

private:
    TArray<FChunkLodData>   LODs;
    uint32                  Initialized;
public:

    void Reset()
    {
        LODs.Empty();
        Initialized = false;
    }

    FORCEINLINE void Initialize(uint8 maxLOD)
    {
        LODs.SetNum(maxLOD + 1);
        Initialized = 0;
    }

    FORCEINLINE void AddNewLOD(uint8 index, FChunkLodData&& data)
    {
        LODs[index] = MoveTemp(data);
        Initialized = Initialized | (1 << index);
    }

    FORCEINLINE bool ContainsLOD(uint8 index)
    {
        return Initialized & (1 << index);
    }

    FORCEINLINE FChunkLodData& GetLOD(uint8 index)
    {
        return LODs[index];
    }

    FORCEINLINE void RemoveLOD(uint8 index)
    {
        delete& LODs[index];
        Initialized = Initialized & (~(1 << index));
    }

    FORCEINLINE int GetMaxLowerLOD(const int32 maxLOD)
    {
        for (int32 i = maxLOD; i > 0; i--)
        {
            if ((1 << i) & Initialized)
            {
                return i;
            }
        }
        return 0;
    }

    FORCEINLINE int GetMinHigherLOD(const int32 maxLOD)
    {
        for (int32 i = maxLOD; i <= LODs.Num(); i++)
        {
            if ((1 << i) & Initialized)
            {
                return i;
            }
        }
        return 0;
    }

};


USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct FChunkPartSelector
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool        downscaled;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    Direction   borderDirection;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    uint8       LOD;

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

    UPROPERTY(EditAnywhere, BlueprintReadonly)
    TArray<uint8> array;

    FArrayUint8()
    {

    }

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


