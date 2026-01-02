#include "ChunkFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

float		UChunkFunctionLibrary::m_noiseScale         = 0.0001f;
float		UChunkFunctionLibrary::m_heightMultiplier   = 2500;
float		UChunkFunctionLibrary::m_chunkWidth         = 12800;
float       UChunkFunctionLibrary::m_UVScale            = 0.1;
uint8       UChunkFunctionLibrary::m_maxLOD             = 8;

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Up(const TArray<FVector>& wholeChunk_additionalsVerts, const uint8 LOD, const bool downscale)
{
    const int32 dataWidth = (1 << m_maxLOD) + 3;
    const int32 step = (1 << (m_maxLOD - LOD));
    const int32 realWidth = (1 << LOD) + 3;
    const int32 edge = step * 3 + 1;

    UE_LOG(LogTemp, Log, TEXT("Step: %d, realWidth: %d"), step, realWidth);

    FMeshData Mesh(
        FVector2D(realWidth, 4),
        false
    );

    int32 realIndx = 0;

    // The first now, we only add verices and UVs, we don't create triangles
    for (int32 X = 0; X < dataWidth - 1; X += step)
    {
        Mesh.vertices.Add(wholeChunk_additionalsVerts[X]);
        Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[X].X, wholeChunk_additionalsVerts[X].Y) * m_UVScale);
        realIndx++;
    }

    Mesh.vertices.Add(wholeChunk_additionalsVerts[dataWidth-1]);
    Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[dataWidth - 1].X, wholeChunk_additionalsVerts[dataWidth - 1].Y) * m_UVScale);
    realIndx++;

    // In the lower parts, we add a vertice, and then, in the inner loop, we add vertice and two corresponding triangle too
    for (int32 Y = 1; Y <= 1; Y += step)
    {
        Mesh.vertices.Add(wholeChunk_additionalsVerts[Y * dataWidth]);
        Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[Y * dataWidth].X, wholeChunk_additionalsVerts[Y * dataWidth].Y) * m_UVScale);
        realIndx++;

        for (int32 X = 1; X < dataWidth - 1; X += step)
        {
            const int32 Indx = Y * dataWidth + X;
            // We use an approximated coord for the vertice if the X axis is even, meaning, the lower LOD would not have a vertice there and it would
            // Just have a lerp coord of the previous and the next vertice
            // This is only for the downscaled case btw
            if (downscale && Y == (step + 1) && ((X - 1) % (step * 2) == step))
            {
                Mesh.vertices.Add(0.5f * (wholeChunk_additionalsVerts[FMath::Max(Indx - step, 0)] +
                    wholeChunk_additionalsVerts[FMath::Min(Indx + step, dataWidth - 1)]
                    ));
            }
            else
            {
                Mesh.vertices.Add(wholeChunk_additionalsVerts[Indx]);
            }
            Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[Indx].X, wholeChunk_additionalsVerts[Indx].Y) * m_UVScale);

            realIndx++;

            const int32 B = realIndx - realWidth;
            const int32 C = B - 1;
            const int32 D = realIndx - 1;

            UE_LOG(LogTemp, Log, TEXT("B: %d, C: %d, D: %d, realIndex: %d"), B, C, D, realIndx);

            Mesh.triangles.Append({ realIndx, B, C,  realIndx, C, D });
        }

        Mesh.vertices.Add(wholeChunk_additionalsVerts[(dataWidth - 1) + Y * dataWidth]);
        Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[(dataWidth - 1) + Y * dataWidth].X, wholeChunk_additionalsVerts[(dataWidth - 1) + Y * dataWidth].Y) * m_UVScale);
        realIndx++;

        const int32 B = realIndx - realWidth;
        const int32 C = B - 1;
        const int32 D = realIndx - 1;
        Mesh.triangles.Append({ realIndx, B, C,  realIndx, C, D });
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh.vertices,
        Mesh.triangles,
        Mesh.UVs,
        Mesh.normals,
        Mesh.tangents
    );

    return Mesh;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Down(
    const TArray<FVector>& wholeChunk_additionalsVerts,
    const uint8 LOD,
    const bool downscale
)
{
    const int32 MaxWidth = (1 << m_maxLOD) + 3;
    const int32 Step = (1 << (m_maxLOD - LOD));
    const int32 RealWidth = (1 << LOD) + 3;

    constexpr int32 TempRows = 4; // neighbor-inner, neighbor-border, this-border, this-inner

    FMeshData Temp(FVector2D(RealWidth, TempRows), false);

    int32 writeIdx = 0;

    // The bottom border starts from the last rows of the grid
    for (int32 r = 0; r < TempRows; ++r)
    {
        // Rows are counted from the bottom
        const int32 baseY = (MaxWidth - 1) - (TempRows - 1) * Step;
        const int32 SrcY = baseY + r * Step;
        const int32 borderRow = r;

        for (int32 c = 0; c < RealWidth; ++c)
        {
            const int32 SrcX = c * Step;
            const int32 SrcIdx = SrcY * MaxWidth + SrcX;

            FVector V = wholeChunk_additionalsVerts[SrcIdx];

            if (downscale && borderRow == 2 && (c & 1))
            {
                const int32 L = SrcIdx - Step;
                const int32 R = SrcIdx + Step;

                V = 0.5f * (
                    wholeChunk_additionalsVerts[L] +
                    wholeChunk_additionalsVerts[R]
                    );
            }

            Temp.vertices.Add(V);
            Temp.UVs.Add(FVector2D(V.X, V.Y) * m_UVScale);

            if (r > 0 && c > 0)
            {
                const int32 A = writeIdx;
                const int32 B = A - RealWidth;
                const int32 C = B - 1;
                const int32 D = A - 1;

                Temp.triangles.Append({ A, B, C, A, C, D });
            }

            ++writeIdx;
        }
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Temp.vertices,
        Temp.triangles,
        Temp.UVs,
        Temp.normals,
        Temp.tangents
    );

    FMeshData Final(FVector2D(RealWidth, 2), true);

    // copy vertices (bottom two rows)
    for (int32 r = 2; r <= 3; ++r)
    {
        for (int32 c = 0; c < RealWidth; ++c)
        {
            const int32 src = r * RealWidth + c;

            Final.vertices.Add(Temp.vertices[src]);
            Final.UVs.Add(Temp.UVs[src]);
            Final.normals.Add(Temp.normals[src]);
            Final.tangents.Add(Temp.tangents[src]);
        }
    }

    // rebuild triangles (2 rows only)
    for (int32 c = 1; c < RealWidth; ++c)
    {
        const int32 A = RealWidth + c;
        const int32 B = c;
        const int32 C = c - 1;
        const int32 D = A - 1;

        Final.triangles.Append({ A, B, C, A, C, D });
    }

    return Final;
}


FMeshData UChunkFunctionLibrary::GetChunkData_Border_Left(const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale)
{
    return *(new FMeshData());
}
FMeshData UChunkFunctionLibrary::GetChunkData_Border_Right(const FMeshData& wholeChunk_additionals_MaxLOD, const uint8 LOD, const bool downscale)
{
    return *(new FMeshData());
}

TArray<float> UChunkFunctionLibrary::GetTopLod_Vertices(const FVector2D& Pos)
{
    const int32 Width = (1 << m_maxLOD) + 1;
    const float Cell = m_chunkWidth / (Width - 1);

    TArray<float> vertices = TArray<float>();
    vertices.Reserve(Width * Width);

    for (int32 Y = 0; Y < Width; ++Y)
    {
        for (int32 X = 0; X < Width; ++X){
            const FVector2D W{ Pos.X + X * Cell, Pos.Y + Y * Cell };

            const float  Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
                * m_heightMultiplier;

            vertices.Emplace(Z);
        }
    }

    return vertices;
}

TArray<FVector> UChunkFunctionLibrary::GetLod_Additionals_Vertices(
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

    TArray<FVector> vertices = TArray<FVector>();
    vertices.Reserve(Width * Width);

    for (int X = 0; X < Width; X++)
    {
        const FVector2D W{ Pivot.X + X * Cell, Pivot.Y};
        const float Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;

        vertices.Add({ W.X, W.Y, Z});
    }

    for (int Y = 1; Y < Width - 1; Y++)
    {
        FVector2D W {Pivot.X, Pivot.Y + Y * Cell};
        float  Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;
        vertices.Add({W.X, W.Y, Z });

        for (int X = 1; X < Width - 1; X++)
        {
            const int32 TopY = step * (Y - 1);
            const int32 TopX = step * (X - 1);
            const int32 Indx = TopY * MaxWidth + TopX;

            vertices.Add({
                Pivot.X + X * Cell,
                Pivot.Y + Y * Cell,
                topLodVertices[Indx]
                });
        }

        W = { Pivot.X + (Width - 1) * Cell, Pivot.Y + Y * Cell};
        Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;

        vertices.Add({ W.X, W.Y, Z });
    }

    for (int X = 0; X < Width; X++)
    {
        const FVector2D W{ Pivot.X + X * Cell, Pivot.Y + (Width - 1) * Cell};
        const float  Z = FMath::PerlinNoise2D(W * m_noiseScale + FVector2D(0.1f))
            * m_heightMultiplier;

        vertices.Add({ W.X, W.Y, Z });
    }

    return vertices;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Center(
    const TArray<FVector>&      wholeChunk_additionals,
    const FVector2D             Pos,
    const int8                  LOD
)
{
    const int32 DataWidth = (1 << LOD) + 3;
    const int32 Width = DataWidth - 4;
    const float Cell = m_chunkWidth / (Width + 2);

    FMeshData* Mesh = new FMeshData(
        FVector2D(DataWidth, DataWidth),
        true
    );

    for (int Y = 0; Y < DataWidth; Y++)
    {
        for (int X = 0; X < DataWidth; X++)
        {
            const int32 Indx = Y * DataWidth + X;

            Mesh->UVs.Add(FVector2D(wholeChunk_additionals[Indx].X, wholeChunk_additionals[Indx].Y) * m_UVScale);
            Mesh->vertices.Add(wholeChunk_additionals[Indx]);

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
    for (int Y = 2; Y < DataWidth - 2; Y++)
    {
        for (int X = 2; X < DataWidth - 2; X++)
        {
            const int32 Indx = Y * DataWidth + X;
    
            Final.vertices.Add(Mesh->vertices[Indx]);
            Final.UVs.Add(Mesh->UVs[Indx]);
            Final.tangents.Add(Mesh->tangents[Indx]);
            Final.normals.Add(Mesh->normals[Indx]);
    
            if (Y > 2 && X > 2)
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
    TArray<float> highRes_vertices = GetTopLod_Vertices(Pos);
    TArray<FVector> wholeChunk_additionals = GetLod_Additionals_Vertices(highRes_vertices, Pos, LOD);
    TArray<FVector> wholeChunk_additionals_maxLOD = GetLod_Additionals_Vertices(highRes_vertices, Pos, m_maxLOD);

    result->Center = GetChunkData_Center(wholeChunk_additionals, Pos, LOD);
    result->borders_normal[static_cast<uint8>(Direction::Up)] = GetChunkData_Border_Up(wholeChunk_additionals_maxLOD, LOD, false);
    result->borders_downscaled[static_cast<uint8>(Direction::Up)] = GetChunkData_Border_Up(wholeChunk_additionals_maxLOD, LOD, true);
    //result->borders_normal[static_cast<uint8>(Direction::Down)] = GetChunkData_Border_Down(wholeChunk_additionals_maxLOD, LOD, false);
    //result->borders_downscaled[static_cast<uint8>(Direction::Down)] = GetChunkData_Border_Down(wholeChunk_additionals_maxLOD, LOD, true);

    /*
        Borders will be implemented here too.
    */

    return *result;
}

