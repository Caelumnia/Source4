#pragma once
#include "GigaMeshData.h"
#include "GigaIndexBuffer.h"

class UGigaMesh;
class UGigaMeshComponent;

class GIGAMESH_API FGigaMeshSceneProxy : public FStaticMeshSceneProxy
{
public:
	FGigaMeshSceneProxy(UGigaMeshComponent* InComponent, UGigaMesh* InMesh);

private:
	mutable TMap<uint64, FGigaIndexBuffer> DynamicIndices;

	bool GetMeshBatch(const FSceneView* View, int32 LODIndex, int32 SectionIndex, uint8 DepthPriorityGroup, FMeshBatch& OutMeshBatch) const;

public:
	// Begin FPrimitiveSceneProxy Interface
	virtual void CreateRenderThreadResources() override;
	virtual void DestroyRenderThreadResources() override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
	                                    FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	// End FPrimitiveSceneProxy Interface
};
