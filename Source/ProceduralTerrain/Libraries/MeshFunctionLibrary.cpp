#include "MeshFunctionLibrary.h"

void UMeshStaticLibrary::CalculateNormals(
	const TArray<FVector>&		Vertices,
	const TArray<int32>&		Indices,
	TArray<FVector>&			OutNormals
)
{
	const int32 VertexCount = Vertices.Num();
	check(Indices.Num() % 3 == 0);

	OutNormals.Init(FVector::ZeroVector, VertexCount);

	for (int32 i = 0; i < Indices.Num(); i += 3)
	{
		const int32 I0 = Indices[i];
		const int32 I1 = Indices[i + 1];
		const int32 I2 = Indices[i + 2];

		const FVector& V0 = Vertices[I0];
		const FVector& V1 = Vertices[I1];
		const FVector& V2 = Vertices[I2];

		FVector FaceNormal = (V2 - V0) ^ (V1 - V0);   // clockwise → outward

		FaceNormal *= -1.0f;

		if (!FaceNormal.IsNearlyZero())
		{
			OutNormals[I0] += FaceNormal;
			OutNormals[I1] += FaceNormal;
			OutNormals[I2] += FaceNormal;
		}
	}

	for (FVector& N : OutNormals)
	{
		N.Normalize(0.0001f);   // Safe‑normalize; leaves zero for isolated verts
	}
}

