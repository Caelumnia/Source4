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

inline uint64 GetCombinedBatchIndex(int32 LODIndex, int32 SectionIndex)
{
	return static_cast<uint64>(LODIndex) << 32 | static_cast<uint64>(SectionIndex);
}

USTRUCT()
struct GIGAMESH_API FGigaBatchMap
{
	GENERATED_BODY()

	void SaveBatch(int32 LODIndex, int32 SectionIndex, FGigaBatch&& Batch)
	{
		uint64 BatchIndex = GetCombinedBatchIndex(LODIndex, SectionIndex);
		if (!Map.Contains(BatchIndex))
		{
			Map.Add(BatchIndex, MoveTemp(Batch));
		}
	}

	const FGigaBatch& GetBatch(int32 LODIndex, int32 SectionIndex) const
	{
		uint64 BatchIndex = GetCombinedBatchIndex(LODIndex, SectionIndex);
		check(Map.Contains(BatchIndex));
		return Map[BatchIndex];
	}
	
	UPROPERTY(VisibleAnywhere)
	TMap<uint64, FGigaBatch> Map;
};
