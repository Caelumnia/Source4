#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FAutoBatchModule : public IModuleInterface
{
public:
	static FAutoBatchModule& Get()
	{
		return FModuleManager::GetModuleChecked<FAutoBatchModule>("AutoBatch");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<SDockTab> CreateTab(const FSpawnTabArgs& Args);
};
