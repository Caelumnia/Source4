#include "GigaMergeTool.h"

#include "ContentBrowserModule.h"
#include "Editor.h"
#include "GigaMergingDialog.h"
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
	{
		return LOCTEXT("GigaMergingToolTooltip", "Same as MeshMerge, but use for huge complex one.");
	}
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
	UE_LOG(LogTemp, Display, TEXT("Start merging..."));
	auto& MergeUtils = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();

	FVector Location;
	TArray<UObject*> Assets;
	{
		FScopedSlowTask SlowTask(0, LOCTEXT("MergingActorsSlowTask", "Merging Actors..."));
		SlowTask.MakeDialog();

		auto SettingsObject = UGigaMergeToolSettings::Get();
		const TArray<UPrimitiveComponent*> MergingComponents = MergingDialog->GetSelectedComponents();

		if (MergingComponents.Num())
		{
			UWorld* World = MergingComponents[0]->GetWorld();
			checkf(World != nullptr, TEXT("Invalid World retrieved from Mesh components"));
			const float ScreenAreaSize = TNumericLimits<float>::Max();

			// If the merge destination package already exists, it is possible that the mesh is already used in a scene somewhere, or its materials or even just its textures.
			// Static primitives uniform buffers could become invalid after the operation completes and lead to memory corruption. To avoid it, we force a global reregister.
			if (FindObject<UObject>(nullptr, *PackageName))
			{
				FGlobalComponentReregisterContext GlobalRegister;
				MergeUtils.MergeComponentsToStaticMesh(MergingComponents, World, SettingsObject->Settings, nullptr, nullptr, PackageName, Assets, Location,
				                                       ScreenAreaSize, true);
			}
			else
			{
				MergeUtils.MergeComponentsToStaticMesh(MergingComponents, World, SettingsObject->Settings, nullptr, nullptr, PackageName, Assets, Location,
				                                       ScreenAreaSize, true);
			}
		}
	}

	if (Assets.Num())
	{
		auto& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		auto& ContentBrowser = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

		for (auto Asset : Assets)
		{
			AssetRegistry.AssetCreated(Asset);
			GEditor->BroadcastObjectReimported(Asset);
		}
		ContentBrowser.Get().SyncBrowserToAssets(Assets, true);
	}

	MergingDialog->Reset();

	return true;
}

#undef LOCTEXT_NAMESPACE
