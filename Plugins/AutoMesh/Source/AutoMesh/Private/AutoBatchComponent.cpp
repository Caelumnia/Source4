#include "AutoBatchComponent.h"

#include "AutoBatchSceneProxy.h"

void UAutoBatchComponent::AddMesh(UAutoMeshComponent* Component)
{
	MeshComponents.Add(Component);
}

FPrimitiveSceneProxy* UAutoBatchComponent::CreateSceneProxy()
{
	if (MeshComponents.Num())
	{
		return new FAutoBatchSceneProxy(this);
	}
	return nullptr;
}
