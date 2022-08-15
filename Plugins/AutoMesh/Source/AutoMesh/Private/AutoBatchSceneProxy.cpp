#include "AutoBatchSceneProxy.h"

FAutoBatchSceneProxy::FAutoBatchSceneProxy(UAutoBatchComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
{
	
}

void FAutoBatchSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	checkSlow(IsInParallelRenderingThread())

	if (HasViewDependentDPG()) return;

	
}

SIZE_T FAutoBatchSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}
