#pragma once
#include "DrawDebugHelpers.h"
#include "GigaMesh.h"
#include "GigaMeshSceneProxy.h"
#include "GigaMeshComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=GigaMesh, meta=(BlueprintSpawnableComponent))
class GIGAMESH_API UGigaMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UGigaMeshComponent() { PrimaryComponentTick.bCanEverTick = true; }
	
	UPROPERTY(EditAnywhere, Category="Debug")
	bool bDrawSubBounds;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override
	{
		if (auto Mesh = GetStaticMesh())
		{
			if (Mesh->IsA<UGigaMesh>())
			{
				return new FGigaMeshSceneProxy(this, Cast<UGigaMesh>(Mesh));
			}
			return Super::CreateSceneProxy();
		}
		return nullptr;
	}
};

inline void UGigaMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bDrawSubBounds)
	{
		auto Mesh = GetStaticMesh();
		if (Mesh && Mesh->IsA<UGigaMesh>())
		{
			auto& BatchMap = CastChecked<UGigaMesh>(Mesh)->BatchMap;
			for (auto& Tuple : BatchMap.Map)
			{
				auto& Batch = Tuple.Value;
				for (auto& Element : Batch.Elements)
				{
					auto SubBounds = Element.Bounds.TransformBy(GetComponentTransform());
					DrawDebugBox(GetWorld(), SubBounds.Origin, SubBounds.BoxExtent, FColor::Blue, false, DeltaTime);
				}
			}
		}
	}
}
