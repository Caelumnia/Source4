#include "GigaMesh.h"

void UGigaMesh::SaveBatch(int32 LODIndex, int32 SectionIndex, FGigaBatch&& Batch)
{
	uint64 BatchIndex = GetBatchIndex(LODIndex, SectionIndex);
	if (!Batches.Contains(BatchIndex))
	{
		Batches.Add(BatchIndex, MoveTemp(Batch));
	}
}
