#pragma once
#include "AutoBatchComponent.h"

class AUTOMESH_API FAutoBatchSceneProxy : public FPrimitiveSceneProxy
{
public:
	FAutoBatchSceneProxy(UAutoBatchComponent* Component);

private:
	TMap<uint32, FBoxSphereBounds> SectionBounds;
	
protected:
	// Begin FPrimitiveSceneProxy Interface
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	virtual SIZE_T GetTypeHash() const override;
	virtual uint32 GetMemoryFootprint() const override { return (sizeof(*this) + GetAllocatedSize()); }
	uint32 GetAllocatedSize() const { return (FPrimitiveSceneProxy::GetAllocatedSize()); }
	// End FPrimitiveSceneProxy Interface
};
