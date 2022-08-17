#include "AutoBatchModule.h"
#include "LevelEditor.h"
#include "SAutoBatchToolbar.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

static const FName AutoBatchTabName("AutoBatch");

#define LOCTEXT_NAMESPACE "FAutoBatchModule"

void FAutoBatchModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AutoBatchTabName,
	                                                  FOnSpawnTab::CreateRaw(this, &FAutoBatchModule::CreateTab))
	                        .SetDisplayName(NSLOCTEXT("AutoBatchModule", "TabTitle", "Auto Batch"))
	                        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory());
}

void FAutoBatchModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AutoBatchTabName);
	}
}

TSharedRef<SDockTab> FAutoBatchModule::CreateTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SAutoBatchToolbar> Toolbar = SNew(SAutoBatchToolbar);

	TSharedRef<SDockTab> DockTab =
		SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Toolbar
		];

	return DockTab;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAutoBatchModule, AutoBatch)
