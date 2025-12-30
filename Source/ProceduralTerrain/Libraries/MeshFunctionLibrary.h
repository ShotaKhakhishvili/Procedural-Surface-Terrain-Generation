// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"

#include "../Structures/MeshData.h"

#include "MeshFunctionLibrary.generated.h"

UCLASS(BlueprintType)
class UMeshStaticLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:

    static void CalculateNormals(
        const TArray<FVector>&          vertices,
        const TArray<int32>&            triangles,
        TArray<FVector>&                normals
    );
};


