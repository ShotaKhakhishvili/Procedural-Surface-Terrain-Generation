// Fill out your copyright notice in the Description page of Project Settings.

#include "TerrainGenerator.h"
#include "Libraries/ChunkFunctionLibrary.h"

void ATerrainGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	m_array_futureMeshDatas.Empty();
	m_array_futureChunkLODs.Empty();

	for (auto& Pair : m_map_chunkComponents)
	{
		if (Pair.Value)
		{
			Pair.Value->DestroyComponent();
		}
	}
	m_map_chunkComponents.Empty();
}

void ATerrainGenerator::Initialize(AActor* observedActor)
{
	m_array_futureMeshDatas.SetNum(m_maxThreads);
	m_array_futureChunkLODs.SetNum(m_maxThreads);
	m_freeThreads = m_maxThreads;
	 
	m_observedActor = observedActor;
	TArray<uint8>	lodMap_horizontal;

	int32 currLod = 0;
	for (int i = 0; i < lodRepetitions.Num(); i++)
	{
		for (int j = 0; j < lodRepetitions[i]; j++)
			lodMap_horizontal.Add(currLod);
		currLod++;
	}
	m_renderHalfWidth = lodMap_horizontal.Num();

	const int renderWidth = m_renderHalfWidth + m_renderHalfWidth;
	const float chunkWidth = UChunkFunctionLibrary::GetChunkWidth();

	m_lodMatrix.SetNum(renderWidth);

	int lodStartIdx = 0;
	for (int32 Y = m_renderHalfWidth - 1; Y >= 0; Y--)
	{
		FArrayUint8 temp = FArrayUint8(renderWidth);

		for (int32 X = 0; X < lodStartIdx; X++)
			temp.array[X] = temp.array[renderWidth - 1 - X] = 0;

		for (int32 X = lodStartIdx; X < m_renderHalfWidth; X++)
		{
			temp.array[X] = temp.array[renderWidth - 1 - X] = lodMap_horizontal[X - lodStartIdx];
		}

		m_lodMatrix[Y].array = m_lodMatrix[m_renderHalfWidth + lodStartIdx].array = temp.array;
		lodStartIdx++;
	}
}

void ATerrainGenerator::Refresh_Datas(
)
{
	uint8 spawnedChunks = 0;
	for (int i = 0; i < m_maxThreads; i++)
	{
		if (m_array_futureMeshDatas[i].IsReady())
		{
			const FVector2D chunkIdx = { m_array_futureChunkLODs[i].X, m_array_futureChunkLODs[i].Y };

			UChunkComponent* chunkComponent;
			if (!m_map_chunkComponents.Contains(chunkIdx))
			{
				chunkComponent = NewObject<UChunkComponent>(this, UChunkComponent::StaticClass());

				chunkComponent->AttachToComponent(
					GetRootComponent(),
					FAttachmentTransformRules::KeepRelativeTransform
				);

				chunkComponent->RegisterComponentWithWorld(GetWorld());

				m_map_chunkComponents.Add(chunkIdx, chunkComponent);
			}
			else
			{
				chunkComponent = m_map_chunkComponents[chunkIdx];
			}

			FChunkLodData* newData = m_array_futureMeshDatas[i].Consume();

			chunkComponent->AddLodData(*(newData),
										m_array_futureChunkLODs[i].Z);

			delete newData;

			m_freeThreads++;

			if (spawnedChunks == m_maxChunkGenerationPerFrame)
				return;
		}
	}
}

FORCEINLINE bool ATerrainGenerator::IsChunkLodGenerated(const FVector2D& chunkIndex, const uint8 LOD)

{
	return(m_map_chunkComponents.Contains(chunkIndex) &&
			m_map_chunkComponents[chunkIndex]->ContainsLOD(LOD));
}

bool ATerrainGenerator::IsChunkLodUnderGeneration(const FVector2D& chunkIndex, const uint8 LOD)

{
	for (int i = 0; i < m_maxThreads; i++)
	{
		if (m_array_futureChunkLODs[i].X == chunkIndex.X &&
			m_array_futureChunkLODs[i].Y == chunkIndex.Y &&
			m_array_futureChunkLODs[i].Z == LOD)
		{
			return true;
		}
	}
	return false;
}

FVector2D ATerrainGenerator::GetClosestCorner()
{
	const FVector2D actorPos = FVector2D(m_observedActor->GetActorLocation().X,
		m_observedActor->GetActorLocation().Y);
	const FVector2D divVal = (actorPos / UChunkFunctionLibrary::GetChunkWidth());

	const float chunkWidth = UChunkFunctionLibrary::GetChunkWidth();

	const FVector2D leftUp = FVector2D(FMath::FloorToFloat(divVal.X), FMath::FloorToFloat(divVal.Y));
	const FVector2D rightUp = leftUp + FVector2D(1, 0);
	const FVector2D leftDown = leftUp + FVector2D(0, 1);
	const FVector2D rightDown = rightUp + FVector2D(0, 1);

	const float dist_leftUp		= FVector2D::DistSquared(actorPos, leftUp * chunkWidth);
	const float dist_rightUp	= FVector2D::DistSquared(actorPos, rightUp * chunkWidth);
	const float dist_leftdown	= FVector2D::DistSquared(actorPos, leftDown * chunkWidth);
	const float dist_rightDown	= FVector2D::DistSquared(actorPos, rightDown * chunkWidth);

	const float dist_min = FMath::Min(FMath::Min(dist_leftUp, dist_rightUp), FMath::Min(dist_leftdown, dist_rightDown));

	if (dist_min == dist_leftUp)			return leftUp;
	else if (dist_min == dist_rightUp)		return rightUp;
	else if (dist_min == dist_leftdown)		return leftDown;
	return rightDown;
}

void ATerrainGenerator::AskToGenerate_Data(
	const FVector2D				chunkIndex,
	const uint8					LOD,
	const bool					forceIfEmptyThread
)
{
	if (!forceIfEmptyThread)
	{		
		// If the data is alread in the queue, we update its LOD.
		if (m_map_chunkDatasToGenerate.Contains(chunkIndex))
		{
			m_map_chunkDatasToGenerate[chunkIndex] = LOD;
		}else // Otherwise we add it into the queue
		{
			m_map_chunkDatasToGenerate.Add(chunkIndex, LOD);
		}
		return;
	}

	// If no free threads, we retun.
	if (m_freeThreads == 0) return;

	// We iterate through the threads, and if their futures are invalid, that means they are free and we add another working proccess
	for (int i = 0; i < m_maxThreads; i++)
	{
		if (!m_array_futureMeshDatas[i].IsValid())
		{
			m_array_futureMeshDatas[i] = Async(EAsyncExecution::ThreadPool, [chunkIndex, LOD]() {
				return &(UChunkFunctionLibrary::GenerateChunkData_LOD(FVector2D(chunkIndex * UChunkFunctionLibrary::GetChunkWidth()), LOD));
				});
			m_map_chunkDatasToGenerate.Remove(chunkIndex);
			m_array_futureChunkLODs[i] = FVector(chunkIndex, LOD);
			m_freeThreads--;
			return;
		}
	}
	return;
}

inline void ATerrainGenerator::AskToGenerate_PossibleData()

{
	if(m_freeThreads == 0 || m_map_chunkDatasToGenerate.IsEmpty())
		return;

	// We just take the first chunk in the queue
	auto entry = m_map_chunkDatasToGenerate.begin();

	// Then force it to be generated, meaning, its not gonna go to the queue, but directly start the generation on some free thread
	AskToGenerate_Data(entry->Key, entry->Value, true);
}

void ATerrainGenerator::AskToDisplayChunks(
)
{
	for (auto it : m_array_visibleChunks)
	{
		it->SetFutureLOD(FChunkLodInfos());
	}

	const int renderWidth = m_renderHalfWidth + m_renderHalfWidth;
	const float chunkWidth = UChunkFunctionLibrary::GetChunkWidth();

	m_array_visibleChunks.Empty(renderWidth * renderWidth);

	const FVector2D startIdx = GetClosestCorner() - m_renderHalfWidth;

	for (int32 Y = 0; Y < renderWidth; Y++)
	{
		for (int32 X = 0; X < renderWidth; X++)
		{
			const FVector2D chunkIdx = startIdx + FVector2D(Y, X);

			if (m_lodMatrix[Y].array[X] > 1)
			{

				if (m_map_chunkComponents.Contains(chunkIdx))
				{
					UChunkComponent* component = m_map_chunkComponents[chunkIdx];
					if (component->ContainsLOD(m_lodMatrix[Y].array[X])) 
					{
						component->SetFutureLOD(FChunkLodInfos(m_lodMatrix[Y].array[X], 0,0,0,0));
						m_array_visibleChunks.Add(component);
					}
					else {
						AskToGenerate_Data(chunkIdx, m_lodMatrix[Y].array[X], false);
						component->SetFutureVisibilityToClosestLOD(m_lodMatrix[Y].array[X]);
					}
				}
				else
				{
					AskToGenerate_Data(chunkIdx, m_lodMatrix[Y].array[X], false);
				}
			}
			else
			{
				if (m_map_chunkComponents.Contains(chunkIdx))
				{
					m_map_chunkComponents[chunkIdx]->SetFutureLOD(FChunkLodInfos());
				}
			}
		}
	}
}


