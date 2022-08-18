#pragma once
#include "CoreMinimal.h"
#include "GigaMeshData.h"
#include "GigaMesh.generated.h"

UCLASS()
class GIGAMESH_API UGigaMesh : public UStaticMesh
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere)
	TMap<uint64, FGigaBatch> Batches;

	void SaveBatch(int32 LODIndex, int32 SectionIndex, FGigaBatch&& Batch);

private:
	uint64 GetBatchIndex(int32 LODIndex, int32 SectionIndex)
	{
		return static_cast<uint64>(LODIndex) << 32 | static_cast<uint64>(SectionIndex);
	}
};
