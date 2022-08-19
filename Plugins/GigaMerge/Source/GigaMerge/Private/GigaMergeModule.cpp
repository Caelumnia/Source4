#include "GigaMergeModule.h"

#include "AssetTypeActions_GigaMesh.h"
#include "GigaMergeTool.h"
#include "IAssetTools.h"
#include "IMergeActorsModule.h"

#define LOCTEXT_NAMESPACE "FGigaMergeModule"

void FGigaMergeModule::StartupModule()
{
	auto& MergeActors = FModuleManager::Get().LoadModuleChecked<IMergeActorsModule>("MergeActors");
	ensure(MergeActors.RegisterMergeActorsTool(MakeUnique<FGigaMergeTool>()));

	auto& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_GigaMesh));
}

void FGigaMergeModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		auto& AssetTools = FModuleManager::Get().GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(MakeShareable(new FAssetTypeActions_GigaMesh));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGigaMergeModule, GigaMerge)
