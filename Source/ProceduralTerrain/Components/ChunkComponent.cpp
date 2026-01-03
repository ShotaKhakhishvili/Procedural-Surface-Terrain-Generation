// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkComponent.h"

UChunkComponent::UChunkComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    m_chunkData.Initialize(UChunkFunctionLibrary::GetMaxLOD());
    m_expectedLodInfos = FChunkLodInfos();
}

void UChunkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    m_chunkData.Reset();
}

void UChunkComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    RefreshChunkVisibility();
}

void UChunkComponent::AddLodData(FChunkLodData& chunkLodData, const uint32 LOD)
{
    CreateNewMeshSection(chunkLodData.Center, FChunkPartSelector(LOD, Direction::Center));

    CreateNewMeshSection(chunkLodData.borders_normal[static_cast<uint32>(Direction::Up)], FChunkPartSelector(LOD, Direction::Up));
    CreateNewMeshSection(chunkLodData.borders_downscaled[static_cast<uint32>(Direction::Up)], FChunkPartSelector(LOD, Direction::Up, true));
    CreateNewMeshSection(chunkLodData.borders_normal[static_cast<uint32>(Direction::Down)], FChunkPartSelector(LOD, Direction::Down));
    CreateNewMeshSection(chunkLodData.borders_downscaled[static_cast<uint32>(Direction::Down)], FChunkPartSelector(LOD, Direction::Down, true));
    CreateNewMeshSection(chunkLodData.borders_normal[static_cast<uint32>(Direction::Left)], FChunkPartSelector(LOD, Direction::Left));
    CreateNewMeshSection(chunkLodData.borders_downscaled[static_cast<uint32>(Direction::Left)], FChunkPartSelector(LOD, Direction::Left, true));
    CreateNewMeshSection(chunkLodData.borders_normal[static_cast<uint32>(Direction::Right)], FChunkPartSelector(LOD, Direction::Right));
    CreateNewMeshSection(chunkLodData.borders_downscaled[static_cast<uint32>(Direction::Right)], FChunkPartSelector(LOD, Direction::Right, true));

    m_chunkData.AddNewLOD(LOD, MoveTemp(chunkLodData));
}

void UChunkComponent::CreateNewMeshSection(const FMeshData& meshData, const FChunkPartSelector& chunkPartSelector)
{
    const uint8 sectionIndex = ConvertPartSelectorToIndex(chunkPartSelector);

    ClearMeshSection(sectionIndex);

    TArray<FColor> VertexColors;
    VertexColors.Init(FColor::White, meshData.vertices.Num());

    CreateMeshSection(
        sectionIndex,                                                   // Section Index
        meshData.vertices,                                              // Vertices
        meshData.triangles,                                             // Triangles (indices)
        meshData.normals,                                               // Normals (can be empty)
        meshData.UVs,                                                   // UV coordinates
        VertexColors,                                                   // Vertex Colors
        meshData.tangents,                                              // Tangents (can be empty)
        (chunkPartSelector.LOD == UChunkFunctionLibrary::GetMaxLOD())   // Enable collision
    );
}

FORCEINLINE uint32 UChunkComponent::ConvertPartSelectorToIndex(const FChunkPartSelector& sel) const
{
    const uint32 MaxLOD = UChunkFunctionLibrary::GetMaxLOD();

    if (sel.borderDirection == Direction::Center)
    {
        return (uint32)sel.LOD;
    }

    const uint32 Base = MaxLOD + 1u;
    const uint32 LODOfs = 8u * (uint32)sel.LOD;

    const uint32 DirOfs = (uint32)sel.borderDirection;      
    const uint32 DownOfs = sel.downscaled ? 4u : 0u;        

    return Base + LODOfs + DirOfs + DownOfs;
}

void UChunkComponent::SetFutureLOD(FChunkLodInfos futureLodInfos)
{
    m_expectedLodInfos = futureLodInfos;
}

void UChunkComponent::RefreshChunkVisibility()
{
    if (m_expectedLodInfos.LOD < 1) return;

    FChunkPartSelector center = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Center);
    FChunkPartSelector up_normal = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Up);
    FChunkPartSelector up_downscaled = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Up, true);
    FChunkPartSelector down_normal = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Down);
    FChunkPartSelector down_downscaled = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Down, true);
    FChunkPartSelector left_normal = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Left);
    FChunkPartSelector left_downscaled = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Left, true);
    FChunkPartSelector right_normal = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Right);
    FChunkPartSelector right_downscaled = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Right, true);

    const int32 sectionIndex_center = ConvertPartSelectorToIndex(center);
    const int32 sectionIndex_up_normal = ConvertPartSelectorToIndex(up_normal);
    const int32 sectionIndex_up_downscaled = ConvertPartSelectorToIndex(up_downscaled);
    const int32 sectionIndex_down_normal = ConvertPartSelectorToIndex(down_normal);
    const int32 sectionIndex_down_downscaled = ConvertPartSelectorToIndex(down_downscaled);
    const int32 sectionIndex_left_normal = ConvertPartSelectorToIndex(left_normal);
    const int32 sectionIndex_left_downscaled = ConvertPartSelectorToIndex(left_downscaled);
    const int32 sectionIndex_right_normal = ConvertPartSelectorToIndex(right_normal);
    const int32 sectionIndex_right_downscaled = ConvertPartSelectorToIndex(right_downscaled);

    const bool donwscaleUp      = m_expectedLodInfos.GetDownscale(Direction::Up);
    const bool donwscaleDowm    = m_expectedLodInfos.GetDownscale(Direction::Down);
    const bool donwscaleLeft    = m_expectedLodInfos.GetDownscale(Direction::Left);
    const bool donwscaleRight   = m_expectedLodInfos.GetDownscale(Direction::Right);

    SetMeshSectionVisible(sectionIndex_center, true);
    SetMeshSectionVisible(sectionIndex_up_normal, !donwscaleUp);
    SetMeshSectionVisible(sectionIndex_up_downscaled, donwscaleUp);
    SetMeshSectionVisible(sectionIndex_down_normal, !donwscaleDowm);
    SetMeshSectionVisible(sectionIndex_down_downscaled, donwscaleDowm);
    SetMeshSectionVisible(sectionIndex_left_normal, !donwscaleLeft);
    SetMeshSectionVisible(sectionIndex_left_downscaled, donwscaleLeft);
    SetMeshSectionVisible(sectionIndex_right_normal, !donwscaleRight);
    SetMeshSectionVisible(sectionIndex_right_downscaled, donwscaleRight);
}

void UChunkComponent::SetFutureVisibilityToClosestLOD(const uint32 lod)
{
    const uint32 maxLowerLOD = m_chunkData.GetMaxLowerLOD(lod);
    const uint32 minHigherLOD = m_chunkData.GetMinHigherLOD(lod);

    const uint32 diffLower = (lod >= maxLowerLOD) ? (lod - maxLowerLOD) : (maxLowerLOD - lod);
    const uint32 diffHigher = (minHigherLOD >= lod) ? (minHigherLOD - lod) : (lod - minHigherLOD);

    m_expectedLodInfos.LOD = (diffLower > diffHigher) ? minHigherLOD : maxLowerLOD;
}
