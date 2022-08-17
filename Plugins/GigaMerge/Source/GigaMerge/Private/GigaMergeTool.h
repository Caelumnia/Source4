#pragma once
#include "IMergeActorsTool.h"
#include "GigaMergeTool.generated.h"

class SGigaMergingDialog;

UCLASS(Config=Engine)
class UGigaMergeToolSettings : public UObject
{
	GENERATED_BODY()
public:
	UGigaMergeToolSettings()
	{
		Settings.bMergePhysicsData = true;
		Settings.LODSelectionType = EMeshLODSelectionType::AllLODs;
	}

	static UGigaMergeToolSettings* Get()
	{
		if (!DefaultSettings)
		{
			DefaultSettings = DuplicateObject(GetMutableDefault<UGigaMergeToolSettings>(), nullptr);
			DefaultSettings->AddToRoot();
		}
		return DefaultSettings;
	}

	static void Destroy()
	{
		if (DefaultSettings)
		{
			DefaultSettings->RemoveFromRoot();
			DefaultSettings->MarkPendingKill();
		}
		DefaultSettings = nullptr;
	}
	
	UPROPERTY(EditAnywhere, meta=(ShowOnlyInnerProperties), Category=MergeSettings)
	FMeshMergingSettings Settings;
	
private:
	static UGigaMergeToolSettings* DefaultSettings;
};

class FGigaMergeTool : public IMergeActorsTool
{
public:
	FGigaMergeTool();
	virtual ~FGigaMergeTool() override;

	//~ Begin IMergeActorsTool
	virtual FName GetIconName() const override { return "MergeActors.MeshMergingTool"; }
	virtual FText GetTooltipText() const override;
	virtual TSharedRef<SWidget> GetWidget() override;
	virtual FString GetDefaultPackageName() const override;
	virtual bool CanMerge() const override;
	virtual bool RunMerge(const FString& PackageName) override;

private:
	TSharedPtr<SGigaMergingDialog> MergingDialog;
	UGigaMergeToolSettings* Settings;
};
