// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "../Structures/MeshData.h"
#include "../Libraries/ChunkFunctionLibrary.h"
#include "ChunkComponent.generated.h"

UCLASS()
class PROCEDURALTERRAIN_API UChunkComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
private:
	FChunkData				m_chunkData;
	FChunkLodInfos			m_expectedLodInfos;
public:

	UChunkComponent(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE bool ContainsLOD(uint32 LOD)
	{
		return m_chunkData.ContainsLOD(LOD);
;	}

	void AddLodData(
		FChunkLodData&		chunkLodData, 
		const uint32			LOD
	);

	void CreateNewMeshSection(const FMeshData&, const FChunkPartSelector&);

	void SetFutureVisibilityToClosestLOD(const uint32 lod);

	void RefreshChunkVisibility();

	FORCEINLINE uint32 ConvertPartSelectorToIndex(const FChunkPartSelector&)const;

	FORCEINLINE void SetFutureLOD(FChunkLodInfos futureBorderInfos);
};
