#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AutoBatchActor.generated.h"

UCLASS(Blueprintable, BlueprintType)
class AUTOMESH_API AAutoBatchActor : public AActor
{
	GENERATED_BODY()

public:
	AAutoBatchActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMesh* StaticMergedMesh;
};
