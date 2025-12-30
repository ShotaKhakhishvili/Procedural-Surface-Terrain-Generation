// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ChunkComponent.h"
#include "Libraries/MeshFunctionLibrary.h"
#include "Libraries/ChunkFunctionLibrary.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/ScopeLock.h"
#include "TerrainGenerator.generated.h"

UCLASS(Blueprintable)
class PROCEDURALTERRAIN_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8											m_maxThreads;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8											m_maxChunkGenerationPerFrame;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int>										lodRepetitions;


private:
	AActor*											m_observedActor;

	uint8											m_freeThreads;
	uint8											m_renderHalfWidth;

	TArray<FArrayUint8>								m_lodMatrix;

	TMap<FVector2D, UChunkComponent*>				m_map_chunkComponents;
	TMap<FVector2D, uint8>							m_map_chunkDatasToGenerate;			//	chunk datas in queue or being generated right now

	TArray<TFuture<FChunkLodData*>>					m_array_futureMeshDatas;			//	chunk datas that are begin generated right now
	TArray<FVector>									m_array_futureChunkLODs;			//	chunk data LODs that are being generated now

	TArray<UChunkComponent*>						m_array_visibleChunks;

public:	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void Initialize(AActor* observedActor);

	UFUNCTION(BlueprintCallable, meta = (ReturnDisplayName = "Success", ToolTip = "Returns true if there was a free thread that started the requested generation"))
	void AskToGenerate_Data(						//	Asks to generate some new chunk data
		const FVector2D				chunkIndex,
		const uint8					LOD,
		const bool					forceIfEmptyThread
	);

	UFUNCTION(BlueprintCallable, meta = (ReturnDisplayName = "Success", ToolTip = "Returns true if there was a free thread that started the requested generation"))
	FORCEINLINE void AskToGenerate_PossibleData();
	
	UFUNCTION(BlueprintCallable)
	void AskToDisplayChunks();						// Checks for the necessary meshes that need to be visible and sets their visibilities				
					 
	UFUNCTION(BlueprintCallable)
	void Refresh_Datas();							// Saves any calculated future mesh data into the chunk components

	FORCEINLINE bool IsChunkLodGenerated(
		const FVector2D&		chunkIndex,
		const uint8				LOD
	);

	bool IsChunkLodUnderGeneration(
		const FVector2D&		chunkIndex,
		const uint8				LOD
	);

	FORCEINLINE FVector2D GetClosestCorner();
};
