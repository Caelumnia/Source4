#include "GigaIndexBuffer.h"
#include "GigaStats.h"

constexpr uint32 StrideSize = sizeof(FGigaIndexBuffer::Stride);

FGigaIndexBuffer::FGigaIndexBuffer(const TArray<uint32>& RawIndices, FGigaBatch&& InBatch, uint32 StartIndex, uint32 NumTriangles)
	: CachedVisibility(0), NumTriangles(0), Batch(InBatch)
{
	const uint32 NumIndices = NumTriangles * 3;
	const uint32* Src = RawIndices.GetData() + StartIndex;
	StaticIndices.SetNum(NumIndices);
	FMemory::Memcpy(StaticIndices.GetData(), Src, NumIndices * StrideSize);
}

void FGigaIndexBuffer::UpdateVisibility(const FConvexVolume& Frustum)
{
	SCOPE_CYCLE_COUNTER(STAT_GigaMesh_UpdateVisibility);
	
	uint64 Visibility = 0;
	TArray<int32> VisibleBatch;
	int32 TotalIndices = 0;
	for (int i = 0; i < Batch.Elements.Num(); ++i)
	{
		auto& Element = Batch.Elements[i];
		if (!Frustum.IntersectBox(Element.Bounds.Origin, Element.Bounds.BoxExtent))
		{
			Visibility |= 1ULL << i;
		}
		else
		{
			VisibleBatch.Add(i);
			TotalIndices += Element.NumTriangles * 3;
		}
	}
	
	if (CachedVisibility == Visibility) return;

	CachedVisibility = Visibility;
	
	if (CachedVisibility == 0)
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
}

void FGigaIndexBuffer::InitRHI()
{
	const uint32 Size = StaticIndices.Num() * StrideSize;
	FRHIResourceCreateInfo CreateInfo;
	void* Buffer = nullptr;
	IndexBufferRHI = RHICreateAndLockIndexBuffer(StrideSize, StaticIndices.Num() * StrideSize, BUF_Static, CreateInfo, Buffer);
	FMemory::Memcpy(Buffer, StaticIndices.GetData(), Size);
	RHIUnlockIndexBuffer(IndexBufferRHI);
	NumTriangles = StaticIndices.Num() / 3;
}

void FGigaIndexBuffer::UpdateRHI(const TArray<Stride>& Indices)
{
	const uint32 Size = Indices.Num() * StrideSize;

	if (Size == 0)
	{
		NumTriangles = 0;
		return;
	}
	
	if (IndexBufferRHI->GetSize() != Size)
	{
		IndexBufferRHI.SafeRelease();
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(StrideSize, Size, BUF_Static, CreateInfo);
	}
	void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Size, RLM_WriteOnly);
	FMemory::Memcpy(Buffer, Indices.GetData(), Size);
	RHIUnlockIndexBuffer(IndexBufferRHI);
	NumTriangles = Indices.Num() / 3;
}
