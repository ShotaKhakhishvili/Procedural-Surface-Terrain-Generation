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

void UChunkComponent::AddLodData(FChunkLodData& chunkLodData, const uint8 LOD)
{
    CreateNewMeshSection(chunkLodData.Center, FChunkPartSelector(LOD, Direction::Center));

    CreateNewMeshSection(chunkLodData.borders_normal[static_cast<uint8>(Direction::Up)], FChunkPartSelector(LOD, Direction::Up));
    //CreateNewMeshSection(chunkLodData.borders_normal[static_cast<uint8>(Direction::Up)], FChunkPartSelector(LOD, Direction::Up, true));

    m_chunkData.AddNewLOD(LOD, MoveTemp(chunkLodData));
}

void UChunkComponent::CreateNewMeshSection(const FMeshData& meshData, const FChunkPartSelector& chunkPartSelector)
{
    const uint8 sectionIndex = ConvertPartSelectorToIndex(chunkPartSelector);

    ClearMeshSection(sectionIndex);

    m_visibleSections.Add(sectionIndex);

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

FORCEINLINE uint32 UChunkComponent::ConvertPartSelectorToIndex(const FChunkPartSelector& sel)const

{
    const uint32 border = UChunkFunctionLibrary::GetMaxLOD() + 1 + 8 * sel.LOD + static_cast<uint8>(sel.borderDirection)  + 4 * sel.downscaled;
    const uint32 ans = (sel.borderDirection == Direction::Center) ? sel.LOD : border;

    return ans;
}

void UChunkComponent::SetFutureLOD(FChunkLodInfos futureLodInfos)
{
    m_expectedLodInfos = futureLodInfos;
}

void UChunkComponent::RefreshChunkVisibility()
{
    for (int32 i = 0; i < m_visibleSections.Num(); i++)
    {
        SetMeshSectionVisible(m_visibleSections[i], false);
    }

    m_visibleSections.Empty(5);

    if (m_expectedLodInfos.LOD < 2) return;

    FChunkPartSelector center = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Center);
    FChunkPartSelector up_normal = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Up);
    FChunkPartSelector up_downscaled = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Up, true);
    FChunkPartSelector down_normal = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Down);
    FChunkPartSelector down_downscaled = FChunkPartSelector(m_expectedLodInfos.LOD, Direction::Down, true);

    const int32 sectionIndex_center = ConvertPartSelectorToIndex(center);
    const int32 sectionIndex_up_normal = ConvertPartSelectorToIndex(up_normal);
    const int32 sectionIndex_up_downscaled = ConvertPartSelectorToIndex(up_downscaled);
    const int32 sectionIndex_down_normal = ConvertPartSelectorToIndex(down_normal);
    const int32 sectionIndex_down_downscaled = ConvertPartSelectorToIndex(down_downscaled);

    SetMeshSectionVisible(sectionIndex_center, true);
    SetMeshSectionVisible(sectionIndex_up_normal, true);
    SetMeshSectionVisible(sectionIndex_up_downscaled, false);
    SetMeshSectionVisible(sectionIndex_down_normal, false);
    SetMeshSectionVisible(sectionIndex_down_downscaled, false);

    m_visibleSections.Add(sectionIndex_center);
    m_visibleSections.Add(sectionIndex_up_normal); 
    //m_visibleSections.Add(sectionIndex_up_downscaled);
    //m_visibleSections.Add(sectionIndex_down_normal);
    //m_visibleSections.Add(sectionIndex_down_downscaled);

    //UE_LOG(LogTemp, Warning, TEXT("Center index : %d"), static_cast<uint8>(sectionIndex_center));
    //UE_LOG(LogTemp, Warning, TEXT("Up normal index : %d"), static_cast<uint8>(sectionIndex_up_normal));
    //UE_LOG(LogTemp, Warning, TEXT("Up downscaled index : %d"), static_cast<uint8>(sectionIndex_up_downscaled));
}

void UChunkComponent::SetFutureVisibilityToClosestLOD(const uint8 lod)
{
    int32 maxLowerLOD = m_chunkData.GetMaxLowerLOD(lod);
    int32 minHigherLOD = m_chunkData.GetMinHigherLOD(lod);

    m_expectedLodInfos.LOD = (lod - maxLowerLOD) > FMath::Abs(minHigherLOD - lod) ? minHigherLOD : maxLowerLOD;
}
