#pragma once
#include "Engine/DataTable.h"
#include "GigaMeshData.generated.h"

USTRUCT(BlueprintType)
struct GIGAMESH_API FGigaBatchElement
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FBoxSphereBounds Bounds;

	UPROPERTY(VisibleAnywhere)
	uint32 FirstIndex;

	UPROPERTY(VisibleAnywhere)
	uint32 NumTriangles;
};

USTRUCT(BlueprintType)
struct GIGAMESH_API FGigaBatch
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<FGigaBatchElement> Elements;
};
