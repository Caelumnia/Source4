#include "AutoBatchActor.h"

// #include "MaterialUtilities.h"
#include "AutoMergeHelper.h"
#include "MaterialUtilities.h"
#include "MeshMergeDataTracker.h"
#include "MeshMergeModule.h"
#include "StaticMeshOperations.h"

AAutoBatchActor::AAutoBatchActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AAutoBatchActor::OnConstruction(const FTransform& Transform)
{
	TArray<USceneComponent*> Components;
	RootComponent->GetChildrenComponents(true, Components);

	TArray<UStaticMeshComponent*> MeshComponents;
	for (auto Child : Components)
	{
		auto Mesh = Cast<UStaticMeshComponent>(Child);
		if (Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetNumSourceModels() > 0)
		{
			MeshComponents.Add(Mesh);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("%d static meshes to merge..."), MeshComponents.Num());

	MeshComponents.RemoveAll([](UStaticMeshComponent* Component)
	{
		return Component->bUseMaxLODAsImposter;
	});

	if (MeshComponents.Num() == 0) return;

	if (StaticMergedMesh)
	{
		StaticMergedMesh->ReleaseResources();
		StaticMergedMesh = nullptr;
	}
	StaticMergedMesh = AutoMergeHelper::MergeComponents(this, MeshComponents, Transform.GetLocation());
}
