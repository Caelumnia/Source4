#pragma once
#include "AutoMeshComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=AutoMesh, meta=(BlueprintSpawnableComponent))
class AUTOMESH_API UAutoMeshComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<FBoxSphereBounds> SectionBounds;

protected:
	virtual void OnComponentCreated() override;
	
private:

public:
	//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface
};
