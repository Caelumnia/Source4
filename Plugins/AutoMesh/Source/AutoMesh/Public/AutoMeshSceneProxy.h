#pragma once

class UAutoMeshComponent;

class AUTOMESH_API FAutoMeshSceneProxy : public FStaticMeshSceneProxy
{
public:
	FAutoMeshSceneProxy(UAutoMeshComponent* InComponent);

private:
	TArray<FBoxSphereBounds> SectionBounds;

	bool CullSection(const FSceneView* View, int32 SectionIndex) const;
	bool GetMeshBatch(int32 LODIndex, int32 SectionIndex, int32 BatchIndex, uint8 DepthPriorityGroup, FMeshBatch& OutMeshBatch) const;
	
public:
	// Begin FPrimitiveSceneProxy Interface
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	// End FPrimitiveSceneProxy Interface
};
