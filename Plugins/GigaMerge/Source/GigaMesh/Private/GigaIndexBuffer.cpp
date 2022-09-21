#include "GigaIndexBuffer.h"
#include "GigaStats.h"

constexpr uint32 StrideSize = sizeof(FGigaIndexBuffer::Stride);

FGigaIndexBuffer::FGigaIndexBuffer(const TArray<uint32>& RawIndices, FGigaBatch&& InBatch, uint32 StartIndex, uint32 NumTriangles)
	: CachedVisibility(true, InBatch.Elements.Num()), NumTriangles(0), Batch(InBatch)
{
	const uint32 NumIndices = NumTriangles * 3;
	const uint32* Src = RawIndices.GetData() + StartIndex;
	StaticIndices.SetNum(NumIndices);
	FMemory::Memcpy(StaticIndices.GetData(), Src, NumIndices * StrideSize);
}

bool FGigaIndexBuffer::UpdateVisibility(const FConvexVolume& Frustum)
{
	SCOPE_CYCLE_COUNTER(STAT_GigaMesh_UpdateVisibility);

	if (!Frustum.IntersectBox(Batch.Bounds.Origin, Batch.Bounds.BoxExtent))
	{
		CachedVisibility.Init(false, Batch.Elements.Num());
		return false;
	}

	const int32 NumElements = Batch.Elements.Num();
	TBitArray<> Visibility(true, NumElements);
	TArray<int32> VisibleBatch;
	int32 TotalIndices = 0;
	for (int i = 0; i < NumElements; ++i)
	{
		auto& Element = Batch.Elements[i];
		if (!Frustum.IntersectBox(Element.Bounds.Origin, Element.Bounds.BoxExtent))
		{
			Visibility[i] = false;
		}
		else
		{
			VisibleBatch.Add(i);
			TotalIndices += Element.NumTriangles * 3;
		}
	}
	
	if (CachedVisibility == Visibility) return true;

	CachedVisibility = Visibility;
	
	if (CachedVisibility.Find(false) == INDEX_NONE)
	{
		UpdateRHI(StaticIndices);
	}
	else
	{
		VisibleIndices.Emplace();
		VisibleIndices.SetNum(TotalIndices);
		uint32 FirstIndex = 0;
		for (int32 Index : VisibleBatch)
		{
			const auto& Element = Batch.Elements[Index];
			const uint32 NumIndices = Element.NumTriangles * 3;
			const uint32* Src = StaticIndices.GetData() + Element.FirstIndex * 3;
			uint32* Dst = VisibleIndices.GetData() + FirstIndex;
			FMemory::Memcpy(Dst, Src, NumIndices * StrideSize);
			FirstIndex += NumIndices;
		}
		UpdateRHI(VisibleIndices);
	}
	return true;
}

void FGigaIndexBuffer::InitRHI()
{
	AllocatedByteCount = StaticIndices.Num() * StrideSize;
	FRHIResourceCreateInfo CreateInfo;
	IndexBufferRHI = RHICreateIndexBuffer(StrideSize, AllocatedByteCount, BUF_Dynamic, CreateInfo);
	UpdateRHI(StaticIndices);
}

void FGigaIndexBuffer::UpdateRHI(const TArray<Stride>& Indices)
{
	check(IsValidRef(IndexBufferRHI));
	check(Indices.Num() <= StaticIndices.Num());
	const uint32 Size = Indices.Num() * StrideSize;

	void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, AllocatedByteCount, RLM_WriteOnly);
	FMemory::Memcpy(Buffer, Indices.GetData(), Size);
	RHIUnlockIndexBuffer(IndexBufferRHI);
	NumTriangles = Indices.Num() / 3;
	
}
