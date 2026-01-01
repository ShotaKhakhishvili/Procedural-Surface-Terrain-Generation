#include "ChunkFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

float		UChunkFunctionLibrary::m_noiseScale         = 0.0001f;
float		UChunkFunctionLibrary::m_heightMultiplier   = 2500;
float		UChunkFunctionLibrary::m_chunkWidth         = 12800;
float       UChunkFunctionLibrary::m_UVScale            = 0.1;
uint8       UChunkFunctionLibrary::m_maxLOD             = 8;

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Up(const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale)
{
    const int32 dataWidth = (1 << m_maxLOD) + 3;
    const int32 step = (1 << (m_maxLOD - LOD));
    const int32 realWidth = (1 << LOD) + 3;
    const int32 edge = step * 4;

    FMeshData Mesh(
        FVector2D(realWidth, 4),
        false
    );

    int32 realIndx = 0;
    for (int32 Y = 0; Y < edge; Y += step)
    {
        Mesh.vertices.Add(wholeChunk_additionals_MaxLOD.vertices[Y * dataWidth]);
        Mesh.UVs.Add(wholeChunk_additionals_MaxLOD.UVs[Y * dataWidth]);
        realIndx++;

        for (int32 X = 1; X < dataWidth - 1; X += step)
        {
            const int32 Indx = Y * dataWidth + X;
            if (downscale && X % (step * 2) && Y < 2 * step)
            {
                Mesh.vertices.Add(0.5f * (wholeChunk_additionals_MaxLOD.vertices[FMath::Max(Indx - step, 0)] +
                    wholeChunk_additionals_MaxLOD.vertices[FMath::Min(Indx + step, dataWidth - 1)]
                    ));
            }
            else
            {
                Mesh.vertices.Add(wholeChunk_additionals_MaxLOD.vertices[Indx]);
            }
            Mesh.UVs.Add(wholeChunk_additionals_MaxLOD.UVs[Indx]);

            if (Y > step)
            {
                const int32 B = realIndx - realWidth;
                const int32 C = B - 1;
                const int32 D = realIndx - 1;
                Mesh.triangles.Append({ realIndx, B, C,  realIndx, C, D });
            }
            realIndx++;
        }

        Mesh.vertices.Add(wholeChunk_additionals_MaxLOD.vertices[(Y + 1) * dataWidth - 1]);
        Mesh.UVs.Add(wholeChunk_additionals_MaxLOD.UVs[(Y + 1) * dataWidth - 1]);
        realIndx++;

        if (Y > step)
        {
            const int32 B = realIndx - realWidth;
            const int32 C = B - 1;
            const int32 D = realIndx - 1;
            Mesh.triangles.Append({ realIndx, B, C,  realIndx, C, D });
        }
    }

    return Mesh;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Down(const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale)
{
    return *(new FMeshData());
}
FMeshData UChunkFunctionLibrary::GetChunkData_Border_Left(const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale)
{
    return *(new FMeshData());
}
FMeshData UChunkFunctionLibrary::GetChunkData_Border_Right(const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale)
{
    return *(new FMeshData());
}

TArray<float>& UChunkFunctionLibrary::GetTopLod_Center_Vertices(const FVector2D& Pos)
{
    const int32 Width = (1 << m_maxLOD) + 1;
    const float Cell = m_chunkWidth / (Width - 1);

    /* ---------- center mesh ---------- */
    TArray<float>* vertices = new TArray<float>();
    vertices->Reserve(Width * Width);

    for (int32 Y = 0; Y < Width; ++Y)
    {
        for (int32 X = 0; X < Width; ++X){
            const FVector2D W{ Pos.X + X * Cell, Pos.Y + Y * Cell };

            const float  Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
                * m_heightMultiplier;

            vertices->Emplace(Z);
        }
    }

    return *vertices;
}

TArray<float>& UChunkFunctionLibrary::GetLod_Additionals_Vertices(
    const TArray<float>&        topLodVertices,
    const FVector2D             Pos,
    const uint8                 LOD
)
{
    const int32 MaxWidth = (1 << m_maxLOD) + 1;
    const int32 Width = (1 << LOD) + 3;

    const float Cell = m_chunkWidth / (Width - 3);
    const int step = (1 << (m_maxLOD - LOD));

    FVector2D Pivot = Pos - FVector2D(Cell);

    /* ---------- outer mesh ---------- */
    TArray<float>* vertices = new TArray<float>();
    vertices->Reserve(Width * Width);

    for (int X = 0; X < Width; X++)
    {
        const FVector2D W{ Pivot.X + X * Cell, Pivot.Y};
        const float Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;

        vertices->Add(Z);
    }

    for (int Y = 1; Y < Width - 1; Y++)
    {
        FVector2D W {Pivot.X, Pivot.Y + Y * Cell};
        float  Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;
        vertices->Add(Z);

        for (int X = 1; X < Width - 1; X ++)
        {
            const int32 TopY = step * (Y - 1);
            const int32 TopX = step * (X - 1);
            const int32 Indx = TopY * MaxWidth + TopX;

            vertices->Add(topLodVertices[Indx]);
        }

        W = { Pivot.X + (Width - 1) * Cell, Pivot.Y + Y * Cell};
        Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;

        vertices->Add(Z);
    }

    for (int X = 0; X < Width; X++)
    {
        const FVector2D W{ Pivot.X + X * Cell, Pivot.Y + (Width - 1) * Cell};
        const float  Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;

        vertices->Add(Z);
    }

    return *vertices;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Border(
    const FMeshData&            wholeChunk,
    const FChunkPartSelector&   chunkPartSelector
)
{
    const int32 outterWidth = (1 << (chunkPartSelector.LOD)) + 3;
    const int32 innerWidth = outterWidth - 2;

    FMeshData* Mesh = new FMeshData(
    );
    switch (chunkPartSelector.borderDirection)
    {
    case Direction::Left:
        *Mesh = GetChunkData_Border_Left(wholeChunk, chunkPartSelector.LOD, chunkPartSelector.downscaled);
        break;
    case Direction::Right:
        *Mesh = GetChunkData_Border_Right(wholeChunk, chunkPartSelector.LOD, chunkPartSelector.downscaled);
        break;
    case Direction::Up:
        *Mesh = GetChunkData_Border_Up(wholeChunk, chunkPartSelector.LOD, chunkPartSelector.downscaled);
        break;
    default:
        *Mesh = GetChunkData_Border_Down(wholeChunk, chunkPartSelector.LOD, chunkPartSelector.downscaled);
        break;
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh->vertices, Mesh->triangles, Mesh->UVs,
        Mesh->normals, Mesh->tangents);

    FMeshData* Final = new FMeshData(
        FVector2D(2, innerWidth),
        true
    );

    for (int X = 1; X < outterWidth - 1; X++)
    {
        int32 Indx = outterWidth + X;

        Final->vertices.Add(Mesh->vertices[Indx]);
        Final->UVs.Add(Mesh->UVs[Indx]);
        Final->normals.Add(Mesh->normals[Indx]);
        Final->tangents.Add(Mesh->tangents[Indx]);
    }
    for (int X = 2; X < outterWidth - 2; X++)
    {
        int32 Indx = 2 * outterWidth + X;

        Final->vertices.Add(Mesh->vertices[Indx]);
        Final->UVs.Add(Mesh->UVs[Indx]);
        Final->normals.Add(Mesh->normals[Indx]);
        Final->tangents.Add(Mesh->tangents[Indx]);
    }
    for (int X = innerWidth; X < 2 * innerWidth - 2; X++)
    {
        const int32 B = X - innerWidth + 1;
        const int32 C = B - 1;
        const int32 D = X - 1;
        Final->triangles.Append({ X, B, C,  X, C, D });
    }

    Final->triangles.Append({innerWidth, 1, 0});
    Final->triangles.Append({2 * innerWidth - 3, innerWidth - 1, innerWidth - 2});

    delete Mesh;
    return *Final;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Center(
    const TArray<float>&        wholeChunk_additionals,
    const FVector2D             Pos,
    const int8                  LOD
)
{
    const int32 DataWidth = (1 << LOD) + 3;
    const int32 Width = DataWidth - 2;
    const float Cell = m_chunkWidth / (Width);

    FMeshData* Mesh = new FMeshData(
        FVector2D(DataWidth, DataWidth),
        true
    );

    for (int Y = 0; Y < DataWidth; Y++)
    {
        for (int X = 0; X < DataWidth; X++)
        {
            const int32 Indx = Y * DataWidth + X;

            const float newZ = wholeChunk_additionals[Indx];

            const FVector2D PosXY = FVector2D(Pos.X + X * Cell, Pos.Y + Y * Cell);

            Mesh->UVs.Add(PosXY * m_UVScale);
            Mesh->vertices.Add(FVector(PosXY, newZ));

            if (X && Y)
            {
                const int32 B = Indx - DataWidth;
                const int32 C = B - 1;
                const int32 D = Indx - 1;

                Mesh->triangles.Append({ Indx, B, C, Indx, C, D });
            }
        }
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh->vertices, Mesh->triangles, Mesh->UVs,
        Mesh->normals, Mesh->tangents
    );

    FMeshData Final(
        FVector2D(Width, Width),
        true
    );
    
    int InnerIndx = 0;
    for (int Y = 1; Y < DataWidth - 1; Y++)
    {
        for (int X = 1; X < DataWidth - 1; X++)
        {
            const int32 Indx = Y * DataWidth + X;
    
            Final.vertices.Add(Mesh->vertices[Indx]);
            Final.UVs.Add(Mesh->UVs[Indx]);
            Final.tangents.Add(Mesh->tangents[Indx]);
            Final.normals.Add(Mesh->normals[Indx]);
    
            if (X > 1 && Y > 1)
            {
                const int32 B = InnerIndx - Width;
                const int32 C = B - 1;
                const int32 D = InnerIndx - 1;
    
                Final.triangles.Append({ InnerIndx, B, C, InnerIndx, C, D });
            }
    
            InnerIndx++;
        }
    }
    
    delete Mesh;

    return Final;
}

FChunkLodData& UChunkFunctionLibrary::GenerateChunkData_LOD(
    const FVector2D&        Pos, 
    const uint8             LOD
)
{
    FChunkLodData* result = new FChunkLodData();
    TArray<float> highRes_vertices = MoveTemp(GetTopLod_Center_Vertices(Pos));
    TArray<float> wholeChunk_additionals = MoveTemp(GetLod_Additionals_Vertices(highRes_vertices, Pos, LOD));

    result->Center = MoveTemp(GetChunkData_Center(wholeChunk_additionals, Pos, LOD));
    //result->borders_normal[static_cast<uint8>(Direction::Up)] = MoveTemp(GetChunkData_Border_Up(wholeChunk_additionals, LOD, false));
    //result->borders_downscaled[static_cast<uint8>(Direction::Up)] = MoveTemp(GetChunkData_Border_Up(wholeChunk_additionals, LOD, true));

    /*
        Borders will be implemented here too.
    */

    return *result;
}

