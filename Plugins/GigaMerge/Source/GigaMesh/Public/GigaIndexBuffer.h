#pragma once
#include "GigaMeshData.h"

class FGigaIndexBuffer : public FIndexBuffer
{
public:
	using Stride = uint32;
	
	FGigaIndexBuffer(const TArray<uint32>& RawIndices, FGigaBatch&& InBatch, uint32 StartIndex, uint32 NumTriangles);

	void UpdateVisibility(const FConvexVolume& Frustum);
	uint32 GetNumTriangles() const { return NumTriangles; }

	virtual void InitRHI() override;
	void UpdateRHI(const TArray<Stride>& Indices);

private:
	TArray<Stride> StaticIndices;
	TArray<Stride> VisibleIndices;
	uint64 CachedVisibility;
	uint32 NumTriangles;
	
	FGigaBatch Batch;
};
