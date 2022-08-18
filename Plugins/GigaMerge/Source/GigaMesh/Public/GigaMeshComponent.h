#pragma once
#include "GigaMesh.h"
#include "GigaMeshSceneProxy.h"
#include "GigaMeshComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=GigaMesh, meta=(BlueprintSpawnableComponent))
class GIGAMESH_API UGigaMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
	
public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override
	{
		if (auto Mesh = GetStaticMesh())
		{
			if (Mesh->IsA<UGigaMesh>())
			{
				return new FGigaMeshSceneProxy(this, Cast<UGigaMesh>(Mesh));
			}
			else
			{
				return Super::CreateSceneProxy();
			}
		}
		return nullptr;
	}
};
