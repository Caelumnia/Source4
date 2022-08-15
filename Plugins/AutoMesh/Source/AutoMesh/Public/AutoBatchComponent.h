#pragma once
#include "AutoBatchComponent.generated.h"

class UAutoMeshComponent;

USTRUCT()
struct FAutoBatchSection
{
	GENERATED_BODY()

	UPROPERTY()
	UStaticMesh* StaticMesh;
	
	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	int32 SectionIndex;

	UPROPERTY()
	bool bVisible;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup=AutoMesh, meta=(BlueprintSpawnableComponent))
class AUTOMESH_API UAutoBatchComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()
public:

	void AddMesh(UAutoMeshComponent* Component);

protected:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

private:
	UPROPERTY(VisibleAnywhere, Category="Batch Info")
	TSet<UAutoMeshComponent*> MeshComponents;

	UPROPERTY(VisibleAnywhere, Category="Batch Info")
	TArray<FAutoBatchSection> Sections;
};
