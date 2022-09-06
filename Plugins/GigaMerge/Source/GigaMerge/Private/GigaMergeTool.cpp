#include "GigaMergeTool.h"

#include "ContentBrowserModule.h"
#include "Editor.h"
#include "GigaMergingDialog.h"
#include "GigaMesh.h"
#include "GigaMeshComponent.h"
#include "GigaMeshData.h"
#include "IContentBrowserSingleton.h"
#include "MeshMergeModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Selection.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "GigaMergingTool"

UGigaMergeToolSettings* UGigaMergeToolSettings::DefaultSettings = nullptr;

FGigaMergeTool::FGigaMergeTool()
{
	Settings = UGigaMergeToolSettings::Get();
}

FGigaMergeTool::~FGigaMergeTool()
{
	UGigaMergeToolSettings::Destroy();
	Settings = nullptr;
}

FText FGigaMergeTool::GetTooltipText() const
{
	return LOCTEXT("GigaMergingToolTooltip", "Merge meshes into a GigaMesh, supporting frustum cull in sections.");
}

TSharedRef<SWidget> FGigaMergeTool::GetWidget()
{
	SAssignNew(MergingDialog, SGigaMergingDialog, this);
	return MergingDialog.ToSharedRef();
}

FString FGigaMergeTool::GetDefaultPackageName() const
{
	FString PackageName = FPackageName::FilenameToLongPackageName(FPaths::ProjectContentDir() + TEXT("SM_BATCHED"));

	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		if (AActor* Actor = Cast<AActor>(*Iter))
		{
			FString ActorName = Actor->GetName();
			PackageName = FString::Printf(TEXT("%s_%s"), *PackageName, *ActorName);
			if (PackageName.Len() > 15) break;
		}
	}

	if (PackageName.IsEmpty())
	{
		PackageName = MakeUniqueObjectName(nullptr, UPackage::StaticClass(), *PackageName).ToString();
	}
	return PackageName;
}

bool FGigaMergeTool::CanMerge() const
{
	return MergingDialog->GetNumSelected() > 1;
}

bool FGigaMergeTool::RunMerge(const FString& PackageName)
{
	FVector Pivot;
	TArray<UObject*> Assets;
	auto Components = MergingDialog->GetSelectedComponents();
	MergeComponents(PackageName, Components, Assets, Pivot);
	checkf(Assets.Num() == 1, TEXT("can't merge into multiple static meshes"));

	UStaticMesh* StaticMesh = CastChecked<UStaticMesh>(Assets[0]);
	UGigaMesh* GigaMesh = DuplicateGigaMesh(GetDefaultAssetPackageName(PackageName), StaticMesh);
	PropagateGigaMesh(Components, Pivot, GigaMesh, StaticMesh);
	Assets.Add(GigaMesh);

	// Save assets
	auto& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	auto& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	for (auto Asset : Assets)
	{
		AssetRegistry.AssetCreated(Asset);
		GEditor->BroadcastObjectReimported(Asset);
	}

	ContentBrowser.Get().SyncBrowserToAssets(Assets, true);

	MergingDialog->Reset();

	return true;
}

FString FGigaMergeTool::GetDefaultAssetPackageName(FString PackageName) const
{
	if (PackageName.IsEmpty()) PackageName = GetDefaultPackageName();
	auto& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	const FString Path = FPackageName::GetLongPackagePath(PackageName);
	FString AssetName = FPackageName::GetShortName(PackageName);
	int32 Prefix = AssetName.Find(TEXT("SM_"), ESearchCase::CaseSensitive);
	if (Prefix != INDEX_NONE)
	{
		AssetName[Prefix] = 'G';
	}
	else
	{
		AssetName.InsertAt(0, FString{TEXT("GM_")});
	}
	FString AssetPackageName = Path + TEXT("/") + AssetName;

	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("CreateMergedActorTitle", "Create Merged GigaMesh");
	SaveAssetDialogConfig.DefaultPath = Path;
	SaveAssetDialogConfig.DefaultAssetName = AssetName;
	SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
	SaveAssetDialogConfig.AssetClassNames = {UGigaMesh::StaticClass()->GetFName()};
	FString SaveObjectPath = ContentBrowser.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
	return SaveObjectPath.IsEmpty() ? AssetPackageName : FPackageName::ObjectPathToPackageName(SaveObjectPath);
}

void FGigaMergeTool::MergeComponents(const FString& PackageName, const TArray<UPrimitiveComponent*>& Components,
                                     TArray<UObject*>& OutAssets, FVector& OutPivot) const
{
	FScopedSlowTask SlowTask(0, LOCTEXT("MergingActorsSlowTask", "Merging Actors..."));
	SlowTask.MakeDialog();

	auto& MergeUtils = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	auto SettingsObject = UGigaMergeToolSettings::Get();
	if (Components.Num())
	{
		UWorld* World = Components[0]->GetWorld();
		checkf(World != nullptr, TEXT("Invalid World retrieved from Mesh components"));
		const float ScreenAreaSize = TNumericLimits<float>::Max();

		// If the merge destination package already exists, it is possible that the mesh is already used in a scene somewhere, or its materials or even just its textures.
		// Static primitives uniform buffers could become invalid after the operation completes and lead to memory corruption. To avoid it, we force a global reregister.
		if (FindObject<UObject>(nullptr, *PackageName))
		{
			FGlobalComponentReregisterContext GlobalRegister;
			MergeUtils.MergeComponentsToStaticMesh(Components, World, SettingsObject->Settings, nullptr, nullptr,
			                                       PackageName, OutAssets, OutPivot, ScreenAreaSize, true);
		}
		else
		{
			MergeUtils.MergeComponentsToStaticMesh(Components, World, SettingsObject->Settings, nullptr, nullptr,
			                                       PackageName, OutAssets, OutPivot, ScreenAreaSize, true);
		}
	}
}

void FGigaMergeTool::PropagateGigaMesh(const TArray<UPrimitiveComponent*>& Components, const FVector& Pivot, UGigaMesh* GigaMesh, UStaticMesh* StaticMesh)
{
	FScopedSlowTask SlowTask(0, LOCTEXT("PropagateGigaMesh", "Propagating Visibility Data"));
	SlowTask.MakeDialog();

	// Create cached resources
	TArray<TArray<FSectionCache>> Resources;
	const int32 NumMergedLODs = StaticMesh->GetNumLODs();
	Resources.SetNum(NumMergedLODs);
	for (int32 LODIndex = 0; LODIndex < NumMergedLODs; ++LODIndex)
	{
		const int32 NumMergedSections = StaticMesh->GetNumSections(LODIndex);
		Resources[LODIndex].SetNum(NumMergedSections);
		for (int32 SectionIndex = 0; SectionIndex < NumMergedSections; ++SectionIndex)
		{
			FStaticMeshSection& Section = StaticMesh->GetRenderData()->LODResources[LODIndex].Sections[SectionIndex];
			FSectionCache& Cache = Resources[LODIndex][SectionIndex];
			Cache.Material = StaticMesh->GetMaterial(Section.MaterialIndex);
			Cache.TotalNumTriangles = Section.NumTriangles;
			Cache.NumElements = 0;
		}
	}

	// Cache components into correct section
	TArray<FTransform> RelativeTransforms;
	FTransform Origin{Pivot};
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ++ComponentIndex)
	{
		auto MeshComponent = CastChecked<UStaticMeshComponent>(Components[ComponentIndex]);
		auto Mesh = MeshComponent->GetStaticMesh();

		RelativeTransforms.Add(MeshComponent->GetComponentTransform().GetRelativeTransform(Origin));

		const int32 NumLODs = Mesh->GetNumLODs();
		for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
		{
			const int32 NumSections = Mesh->GetNumSections(LODIndex);
			for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
			{
				FStaticMeshSection& Section = Mesh->GetRenderData()->LODResources[LODIndex].Sections[SectionIndex];
				UMaterialInterface* Material = MeshComponent->GetMaterial(Section.MaterialIndex);
				for (auto& Info : Resources[LODIndex])
				{
					if (Material->GetName() == Info.Material->GetName())
					{
						Info.ComponentIndex.Add(ComponentIndex);
						Info.NumTriangles.Add(Section.NumTriangles);
					}
					else
					{
						Info.ComponentIndex.Add(INDEX_NONE);
						Info.NumTriangles.Add(0);
					}
					Info.NumElements++;
				}
			}
		}
	}

	// Propagate data
	for (int32 LODIndex = 0; LODIndex < NumMergedLODs; ++LODIndex)
	{
		const int32 NumMergedSections = StaticMesh->GetNumSections(LODIndex);
		uint32 FirstIndex = 0;
		for (int32 SectionIndex = 0; SectionIndex < NumMergedSections; ++SectionIndex)
		{
			auto& Cache = Resources[LODIndex][SectionIndex];

			FGigaBatch Batch;
			for (int ElementIndex = 0; ElementIndex < Cache.NumElements; ++ElementIndex)
			{
				const int32 ComponentIndex = Cache.ComponentIndex[ElementIndex];
				if (ComponentIndex == INDEX_NONE) continue;

				FGigaBatchElement Element;
				auto Component = CastChecked<UStaticMeshComponent>(Components[ComponentIndex]);
				if (auto Mesh = Cast<UGigaMesh>(Component->GetStaticMesh())) // combine another GigaMesh
				{
					const auto& Batches = Mesh->BatchMap;
					// TODO: Merge GigaMesh
				}
				else
				{
					Element.Bounds = Component->GetStaticMesh()->GetBounds().TransformBy(RelativeTransforms[ComponentIndex]);
					Element.FirstIndex = FirstIndex;
					Element.NumTriangles = Cache.NumTriangles[ElementIndex];

					FirstIndex += Element.NumTriangles;
					Batch.Elements.Add(MoveTemp(Element));
				}
			}
			GigaMesh->BatchMap.SaveBatch(LODIndex, SectionIndex, MoveTemp(Batch));
		}
	}
}

UGigaMesh* FGigaMergeTool::DuplicateGigaMesh(FString&& AssetName, UStaticMesh* StaticMesh) const
{
	// Create package for new asset
	UPackage* Package = CreatePackage(*AssetName);
	check(Package);
	Package->FullyLoad();
	Package->Modify();

	FObjectDuplicationParameters DupParams(StaticMesh, Package);
	DupParams.DestClass = UGigaMesh::StaticClass();
	DupParams.DestName = FPackageName::GetShortFName(AssetName);
	return CastChecked<UGigaMesh>(StaticDuplicateObjectEx(DupParams));
}

#undef LOCTEXT_NAMESPACE
