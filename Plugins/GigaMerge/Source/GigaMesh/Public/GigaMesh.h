#pragma once
#include "CoreMinimal.h"
#include "GigaMeshData.h"
#include "GigaMesh.generated.h"

UCLASS(HideCategories=Object, BlueprintType, Config=Engine)
class GIGAMESH_API UGigaMesh : public UStaticMesh
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, Category="GigaMesh")
	FGigaBatchMap BatchMap;

	virtual void Serialize(FArchive& Ar) override;
};

inline void UGigaMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}
