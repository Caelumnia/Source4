#include "AutoMeshComponent.h"
#include "AutoMeshSceneProxy.h"

void UAutoMeshComponent::OnComponentCreated()
{
	const float Width = 100.0f;
	FVector Center{0.0, 0.0, Width + Width / 2.0f};
	FVector Extent{Width / 2.0f};
	SectionBounds = {
		FBoxSphereBounds{Center, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0, Width, 0.0}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0,-Width, 0.0}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0,-Width, Width}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0, 0.0, Width}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0, Width, Width}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0, 0.0, -Width}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0, Width,-Width}, Extent, Width},
		FBoxSphereBounds{Center + FVector{0.0,-Width,-Width}, Extent, Width},
	};
	Super::OnComponentCreated();
}

FPrimitiveSceneProxy* UAutoMeshComponent::CreateSceneProxy()
{
	if (GetStaticMesh() == nullptr || GetStaticMesh()->GetRenderData() == nullptr)
	{
		return nullptr;
	}

	const FStaticMeshLODResourcesArray& LODResources = GetStaticMesh()->GetRenderData()->LODResources;
	if (LODResources.Num() == 0	|| LODResources[FMath::Clamp<int32>(GetStaticMesh()->GetMinLOD().Default, 0, LODResources.Num()-1)].VertexBuffers.StaticMeshVertexBuffer.GetNumVertices() == 0)
	{
		return nullptr;
	}
	LLM_SCOPE(ELLMTag::StaticMesh);

	FPrimitiveSceneProxy* Proxy = ::new FAutoMeshSceneProxy(this);
#if STATICMESH_ENABLE_DEBUG_RENDERING
	SendRenderDebugPhysics(Proxy);
#endif

	return Proxy;
}

/*
void UAutoMeshComponent::OnRegister()
{
	Super::OnRegister();
	TArray<USceneComponent*> Parents;
	GetParentComponents(Parents);
	for (int i = 0; i < Parents.Num(); ++i)
	{
		if (auto AutoBatch = Cast<UAutoBatchComponent>(Parents[i]))
		{
			AutoBatch->AddMesh(this);
			break;
		}
	}
}
*/
