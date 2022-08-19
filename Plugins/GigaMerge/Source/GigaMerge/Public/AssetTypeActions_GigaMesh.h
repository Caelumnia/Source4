#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "GigaMesh.h"
#include "EditorFramework/ThumbnailInfo.h"
#include "StaticMeshEditor/Public/StaticMeshEditorModule.h"

class FAssetTypeActions_GigaMesh : public FAssetTypeActions_Base
{
public:
	// Begin IAssetTypeActions Interface
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_GigaMesh", "Giga Mesh"); }
	virtual FColor GetTypeColor() const override { return FColor{0, 255, 255}; }
	virtual UClass* GetSupportedClass() const override { return UGigaMesh::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }
	virtual bool IsImportedAsset() const override { return true; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
	virtual UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;
	// End IAssetTypeActions Interface
};

inline void FAssetTypeActions_GigaMesh::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	auto& StaticMeshEditor = FModuleManager::Get().LoadModuleChecked<IStaticMeshEditorModule>("StaticMeshEditor");
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	
	for (auto Obj : InObjects)
	{
		if (auto Mesh = Cast<UStaticMesh>(Obj))
		{
			StaticMeshEditor.CreateStaticMeshEditor(Mode, EditWithinLevelEditor, Mesh);
		}
	}
}

inline UThumbnailInfo* FAssetTypeActions_GigaMesh::GetThumbnailInfo(UObject* Asset) const
{
	UStaticMesh* StaticMesh = CastChecked<UStaticMesh>(Asset);
	UThumbnailInfo* ThumbnailInfo = StaticMesh->ThumbnailInfo;
	if (!ThumbnailInfo)
	{
		ThumbnailInfo = NewObject<UThumbnailInfo>(StaticMesh, NAME_None, RF_Transactional);
		StaticMesh->ThumbnailInfo = ThumbnailInfo;
	}
	return ThumbnailInfo;
}
