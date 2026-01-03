#include "ChunkFunctionLibrary.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

float		UChunkFunctionLibrary::m_noiseScale         = 0.0001f;
float		UChunkFunctionLibrary::m_heightMultiplier   = 2500;
float		UChunkFunctionLibrary::m_chunkWidth         = 12800;
float       UChunkFunctionLibrary::m_UVScale            = 0.1;
uint8       UChunkFunctionLibrary::m_maxLOD             = 8;

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Up(
    const TArray<FVector>&  wholeChunk_additionalsVerts, 
    const uint8             LOD, 
    const bool              downscale
)
{
    const int32 dataWidth = (1 << m_maxLOD) + 3;
    const int32 step = (1 << (m_maxLOD - LOD));
    const int32 realWidth = (1 << LOD) + 3;
    const int32 edge = step * 3 + 1;

    FMeshData Mesh(
        FVector2D(realWidth, 4),
        false
    );

    int32 realIndx = 1;

    Mesh.vertices.Add(wholeChunk_additionalsVerts[0]);
    Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[0].X, wholeChunk_additionalsVerts[0].Y) * m_UVScale);

    // The first now, we only add verices and UVs, we don't create triangles
    for (int32 X = 1; X < dataWidth - 1; X += step)
    {
        Mesh.vertices.Add(wholeChunk_additionalsVerts[X]);
        Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[X].X, wholeChunk_additionalsVerts[X].Y) * m_UVScale);
    }

    Mesh.vertices.Add(wholeChunk_additionalsVerts[dataWidth - 1]);
    Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[dataWidth - 1].X, wholeChunk_additionalsVerts[dataWidth - 1].Y) * m_UVScale);


    // In the lower parts, we add a vertice, and then, in the inner loop, we add vertice and two corresponding triangle too
    for (int32 Y = 1; Y <= edge; Y += step)
    {
        Mesh.vertices.Add(wholeChunk_additionalsVerts[Y * dataWidth]);
        Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[Y * dataWidth].X, wholeChunk_additionalsVerts[Y * dataWidth].Y) * m_UVScale);

        for (int32 X = 1; X < dataWidth - 1; X += step)
        {
            const int32 Indx = Y * dataWidth + X;
            // We use an approximated coord for the vertice if the X axis is even, meaning, the lower LOD would not have a vertice there and it would
            // Just have a lerp coord of the previous and the next vertice
            // This is only for the downscaled case btw
            if (downscale && Y == 1 && ((X - 1) % (step * 2) == step))
            {
                const int32 Lx = FMath::Clamp(X - step, 0, dataWidth - 1);
                const int32 Rx = FMath::Clamp(X + step, 0, dataWidth - 1);

                const FVector V =
                    0.5f * (wholeChunk_additionalsVerts[Y * dataWidth + Lx] +
                        wholeChunk_additionalsVerts[Y * dataWidth + Rx]);

                Mesh.vertices.Add(V);
                Mesh.UVs.Add(FVector2D(V.X, V.Y) * m_UVScale);
            }
            else
            {
                Mesh.vertices.Add(wholeChunk_additionalsVerts[Indx]);
                Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[Indx].X, wholeChunk_additionalsVerts[Indx].Y) * m_UVScale);
            }
        }

        Mesh.vertices.Add(wholeChunk_additionalsVerts[(dataWidth - 1) + Y * dataWidth]);
        Mesh.UVs.Add(FVector2D(wholeChunk_additionalsVerts[(dataWidth - 1) + Y * dataWidth].X, wholeChunk_additionalsVerts[(dataWidth - 1) + Y * dataWidth].Y) * m_UVScale);
    }

    for (int32 i = 1; i < 4; i++)
    {
        for (int32 j = 1; j < realWidth; j++)
        {
            const int32 B = (i - 1) * realWidth + j;
            const int32 C = B - 1;
            const int32 D = i * realWidth + j - 1;
            const int32 A = D + 1;
    
            Mesh.triangles.Append({ A, B, C, A, C, D });
        }
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh.vertices,
        Mesh.triangles,
        Mesh.UVs,
        Mesh.normals,
        Mesh.tangents
    );

    FMeshData Final = FMeshData(FVector2D(realWidth, 2), true);

    for (int32 i = realWidth + 1; i < 2 * realWidth - 1; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    for (int32 i = 2 * realWidth + 2; i < 3 * realWidth - 2; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    for (int32 i = realWidth - 2; i < 2 * realWidth - 7; i++)
    {
        const int32 A = i - (realWidth - 3);
        const int32 B = A + 1;
        const int32 C = i;

        Final.triangles.Append({ C, i + 1, B, A, C, B });
    }

    Final.triangles.Append({ 1, 0, realWidth - 2, realWidth -3, realWidth-4, 2 * realWidth - 7 });

    return Final;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Down(
    const TArray<FVector>&  wholeChunk_additionalsVerts,
    const uint8             LOD,
    const bool              downscale
)
{
    const int32 dataWidth = (1 << m_maxLOD) + 3;
    const int32 step = (1 << (m_maxLOD - LOD));
    const int32 realWidth = (1 << LOD) + 3;
    const int32 edge = step * 3 + 1;

    FMeshData Mesh(FVector2D(realWidth, 4), false);

    auto AddVU = [&](const FVector& V)
        {
            Mesh.vertices.Add(V);
            Mesh.UVs.Add(FVector2D(V.X, V.Y) * m_UVScale);
        };

    // LocalY goes "upward from bottom border" (0..edge) -> SrcY goes (dataWidth-1 .. downwards)
    auto SrcYFromLocalY = [&](int32 LocalY) -> int32
        {
            return (dataWidth - 1) - LocalY;
        };

    // ---- Row 0 (outermost bottom border row) ----
    {
        const int32 SrcY = dataWidth - 1;

        AddVU(wholeChunk_additionalsVerts[SrcY * dataWidth + 0]);

        for (int32 X = 1; X < dataWidth - 1; X += step)
        {
            AddVU(wholeChunk_additionalsVerts[SrcY * dataWidth + X]);
        }

        AddVU(wholeChunk_additionalsVerts[SrcY * dataWidth + (dataWidth - 1)]);
    }

    // ---- Next rows going upward from the bottom border region ----
    const int32 MaxLocalY = FMath::Min(edge, dataWidth - 1); // safety
    for (int32 LocalY = 1; LocalY <= MaxLocalY; LocalY += step)
    {
        const int32 SrcY = SrcYFromLocalY(LocalY);

        // left edge
        AddVU(wholeChunk_additionalsVerts[SrcY * dataWidth + 0]);

        for (int32 X = 1; X < dataWidth - 1; X += step)
        {
            const int32 SrcIdx = SrcY * dataWidth + X;

            // Same stitch condition as Up (just on the "first inner stitch row" of this border patch)
            if (downscale && LocalY == 1 && ((X - 1) % (step * 2) == step))
            {
                const int32 Lx = FMath::Clamp(X - step, 0, dataWidth - 1);
                const int32 Rx = FMath::Clamp(X + step, 0, dataWidth - 1);

                const FVector V =
                    0.5f * (wholeChunk_additionalsVerts[SrcY * dataWidth + Lx] +
                        wholeChunk_additionalsVerts[SrcY * dataWidth + Rx]);

                AddVU(V); // UV from V
            }
            else
            {
                AddVU(wholeChunk_additionalsVerts[SrcIdx]);
            }
        }

        // right edge
        AddVU(wholeChunk_additionalsVerts[SrcY * dataWidth + (dataWidth - 1)]);
    }

    for (int32 i = 1; i < 4; i++)
    {
        for (int32 j = 1; j < realWidth; j++)
        {
            const int32 B = (i - 1) * realWidth + j;
            const int32 C = B - 1;
            const int32 D = i * realWidth + j - 1;
            const int32 A = D + 1;

            // reversed winding vs Up
            Mesh.triangles.Append({ A, C, B,  A, D, C });
        }
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh.vertices,
        Mesh.triangles,
        Mesh.UVs,
        Mesh.normals,
        Mesh.tangents
    );

    FMeshData Final(FVector2D(realWidth, 2), true);

    for (int32 i = realWidth + 1; i < 2 * realWidth - 1; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    for (int32 i = 2 * realWidth + 2; i < 3 * realWidth - 2; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    for (int32 i = realWidth - 2; i < 2 * realWidth - 7; i++)
    {
        const int32 A = i - (realWidth - 3);
        const int32 B = A + 1;
        const int32 C = i;

        Final.triangles.Append({ C, B, i + 1,  A, B, C });
    }

    Final.triangles.Append({
        1, realWidth - 2, 0,
        realWidth - 3, 2 * realWidth - 7, realWidth - 4
        });

    return Final;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Left(
    const TArray<FVector>& wholeChunk_additionalsVerts,
    const uint8            LOD,
    const bool             downscale
)
{
    const int32 dataWidth = (1 << m_maxLOD) + 3;
    const int32 step = (1 << (m_maxLOD - LOD));
    const int32 realWidth = (1 << LOD) + 3;
    const int32 edge = step * 3 + 1;

    FMeshData Mesh(FVector2D(realWidth, 4), false);

    auto AddVU = [&](const FVector& V)
        {
            Mesh.vertices.Add(V);
            Mesh.UVs.Add(FVector2D(V.X, V.Y) * m_UVScale);
        };

    // LocalX goes "rightward from left border" (0..edge) -> SrcX is the same
    auto SrcXFromLocalX = [&](int32 LocalX) -> int32
        {
            return LocalX;
        };

    // ---- Row 0 (outermost left border column) ----
    {
        const int32 SrcX = 0;

        AddVU(wholeChunk_additionalsVerts[0 * dataWidth + SrcX]);

        for (int32 Y = 1; Y < dataWidth - 1; Y += step)
        {
            AddVU(wholeChunk_additionalsVerts[Y * dataWidth + SrcX]);
        }

        AddVU(wholeChunk_additionalsVerts[(dataWidth - 1) * dataWidth + SrcX]);
    }

    // ---- Next rows going inward from the left border region ----
    const int32 MaxLocalX = FMath::Min(edge, dataWidth - 1);
    for (int32 LocalX = 1; LocalX <= MaxLocalX; LocalX += step)
    {
        const int32 SrcX = SrcXFromLocalX(LocalX);

        // top
        AddVU(wholeChunk_additionalsVerts[0 * dataWidth + SrcX]);

        for (int32 Y = 1; Y < dataWidth - 1; Y += step)
        {
            const int32 SrcIdx = Y * dataWidth + SrcX;

            // Stitch on the "first inner stitch column" of this border patch
            if (downscale && LocalX == 1 && ((Y - 1) % (step * 2) == step))
            {
                const int32 Uy = FMath::Clamp(Y - step, 0, dataWidth - 1);
                const int32 Dy = FMath::Clamp(Y + step, 0, dataWidth - 1);

                const FVector V =
                    0.5f * (wholeChunk_additionalsVerts[Uy * dataWidth + SrcX] +
                        wholeChunk_additionalsVerts[Dy * dataWidth + SrcX]);

                AddVU(V); // UV from V
            }
            else
            {
                AddVU(wholeChunk_additionalsVerts[SrcIdx]);
            }
        }

        // bottom
        AddVU(wholeChunk_additionalsVerts[(dataWidth - 1) * dataWidth + SrcX]);
    }

    // NOTE: winding flipped (your triangles were reversed)
    for (int32 i = 1; i < 4; i++)
    {
        for (int32 j = 1; j < realWidth; j++)
        {
            const int32 B = (i - 1) * realWidth + j;
            const int32 C = B - 1;
            const int32 D = i * realWidth + j - 1;
            const int32 A = D + 1;

            Mesh.triangles.Append({ A, C, B,  A, D, C });
        }
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh.vertices,
        Mesh.triangles,
        Mesh.UVs,
        Mesh.normals,
        Mesh.tangents
    );

    FMeshData Final(FVector2D(realWidth, 2), true);

    for (int32 i = realWidth + 1; i < 2 * realWidth - 1; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    for (int32 i = 2 * realWidth + 2; i < 3 * realWidth - 2; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    // NOTE: winding flipped (your triangles were reversed)
    for (int32 i = realWidth - 2; i < 2 * realWidth - 7; i++)
    {
        const int32 A = i - (realWidth - 3);
        const int32 B = A + 1;
        const int32 C = i;

        Final.triangles.Append({ C, B, i + 1,  A, B, C });
    }

    Final.triangles.Append({
        1, realWidth - 2, 0,
        realWidth - 3, 2 * realWidth - 7, realWidth - 4
        });

    return Final;
}

FMeshData UChunkFunctionLibrary::GetChunkData_Border_Right(
    const TArray<FVector>& wholeChunk_additionalsVerts,
    const uint8            LOD,
    const bool             downscale
)
{
    const int32 dataWidth = (1 << m_maxLOD) + 3;
    const int32 step = (1 << (m_maxLOD - LOD));
    const int32 realWidth = (1 << LOD) + 3;
    const int32 edge = step * 3 + 1;

    FMeshData Mesh(FVector2D(realWidth, 4), false);

    auto AddVU = [&](const FVector& V)
        {
            Mesh.vertices.Add(V);
            Mesh.UVs.Add(FVector2D(V.X, V.Y) * m_UVScale);
        };

    // LocalX goes "leftward from right border" (0..edge) -> SrcX goes (dataWidth-1 .. downwards)
    auto SrcXFromLocalX = [&](int32 LocalX) -> int32
        {
            return (dataWidth - 1) - LocalX;
        };

    // ---- Row 0 (outermost right border column) ----
    {
        const int32 SrcX = dataWidth - 1;

        AddVU(wholeChunk_additionalsVerts[0 * dataWidth + SrcX]);

        for (int32 Y = 1; Y < dataWidth - 1; Y += step)
        {
            AddVU(wholeChunk_additionalsVerts[Y * dataWidth + SrcX]);
        }

        AddVU(wholeChunk_additionalsVerts[(dataWidth - 1) * dataWidth + SrcX]);
    }

    // ---- Next rows going inward from the right border region ----
    const int32 MaxLocalX = FMath::Min(edge, dataWidth - 1);
    for (int32 LocalX = 1; LocalX <= MaxLocalX; LocalX += step)
    {
        const int32 SrcX = SrcXFromLocalX(LocalX);

        // top
        AddVU(wholeChunk_additionalsVerts[0 * dataWidth + SrcX]);

        for (int32 Y = 1; Y < dataWidth - 1; Y += step)
        {
            const int32 SrcIdx = Y * dataWidth + SrcX;

            // Stitch on the "first inner stitch column" of this border patch
            if (downscale && LocalX == 1 && ((Y - 1) % (step * 2) == step))
            {
                const int32 Uy = FMath::Clamp(Y - step, 0, dataWidth - 1);
                const int32 Dy = FMath::Clamp(Y + step, 0, dataWidth - 1);

                const FVector V =
                    0.5f * (wholeChunk_additionalsVerts[Uy * dataWidth + SrcX] +
                        wholeChunk_additionalsVerts[Dy * dataWidth + SrcX]);

                AddVU(V); // UV from V
            }
            else
            {
                AddVU(wholeChunk_additionalsVerts[SrcIdx]);
            }
        }

        // bottom
        AddVU(wholeChunk_additionalsVerts[(dataWidth - 1) * dataWidth + SrcX]);
    }

    // NOTE: winding flipped back (your triangles were reversed)
    for (int32 i = 1; i < 4; i++)
    {
        for (int32 j = 1; j < realWidth; j++)
        {
            const int32 B = (i - 1) * realWidth + j;
            const int32 C = B - 1;
            const int32 D = i * realWidth + j - 1;
            const int32 A = D + 1;

            Mesh.triangles.Append({ A, B, C,  A, C, D });
        }
    }

    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Mesh.vertices,
        Mesh.triangles,
        Mesh.UVs,
        Mesh.normals,
        Mesh.tangents
    );

    FMeshData Final(FVector2D(realWidth, 2), true);

    for (int32 i = realWidth + 1; i < 2 * realWidth - 1; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    for (int32 i = 2 * realWidth + 2; i < 3 * realWidth - 2; i++)
    {
        Final.vertices.Add(Mesh.vertices[i]);
        Final.UVs.Add(Mesh.UVs[i]);
        Final.tangents.Add(Mesh.tangents[i]);
        Final.normals.Add(Mesh.normals[i]);
    }

    // NOTE: winding flipped back (your triangles were reversed)
    for (int32 i = realWidth - 2; i < 2 * realWidth - 7; i++)
    {
        const int32 A = i - (realWidth - 3);
        const int32 B = A + 1;
        const int32 C = i;

        Final.triangles.Append({ C, i + 1, B,  A, C, B });
    }

    Final.triangles.Append({ 1, 0, realWidth - 2,  realWidth - 3, realWidth - 4, 2 * realWidth - 7 });

    return Final;
}

TArray<float> UChunkFunctionLibrary::GetTopLod_Vertices(
    const FVector2D&    Pos
)
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
    result->borders_normal[static_cast<uint8>(Direction::Down)] = GetChunkData_Border_Down(wholeChunk_additionals_maxLOD, LOD, false);
    result->borders_downscaled[static_cast<uint8>(Direction::Down)] = GetChunkData_Border_Down(wholeChunk_additionals_maxLOD, LOD, true);
    result->borders_normal[static_cast<uint8>(Direction::Left)] = GetChunkData_Border_Left(wholeChunk_additionals_maxLOD, LOD, false);
    result->borders_downscaled[static_cast<uint8>(Direction::Left)] = GetChunkData_Border_Left(wholeChunk_additionals_maxLOD, LOD, true);
    result->borders_normal[static_cast<uint8>(Direction::Right)] = GetChunkData_Border_Right(wholeChunk_additionals_maxLOD, LOD, false);
    result->borders_downscaled[static_cast<uint8>(Direction::Right)] = GetChunkData_Border_Right(wholeChunk_additionals_maxLOD, LOD, true);

    return *result;
}

