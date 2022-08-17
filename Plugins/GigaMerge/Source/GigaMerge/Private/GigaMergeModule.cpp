#include "GigaMergeModule.h"

#include "GigaMergeTool.h"
#include "IMergeActorsModule.h"

#define LOCTEXT_NAMESPACE "FGigaMergeModule"

void FGigaMergeModule::StartupModule()
{
	auto& MergeActorsModule = FModuleManager::Get().LoadModuleChecked<IMergeActorsModule>("MergeActors");
	ensure(MergeActorsModule.RegisterMergeActorsTool(MakeUnique<FGigaMergeTool>()));
}

void FGigaMergeModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGigaMergeModule, GigaMerge)
