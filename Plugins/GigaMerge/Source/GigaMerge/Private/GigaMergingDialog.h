#pragma once

class FGigaMergeTool;

struct FComponentData
{
	TWeakObjectPtr<UStaticMeshComponent> Component;
	bool bEnable;

	FComponentData(UStaticMeshComponent* InComponent) : Component(InComponent), bEnable(true) {}
};

struct FSelectionList
{
	TArray<TSharedPtr<FComponentData>> SelectedComponents;
	TSharedPtr<SListView<TSharedPtr<FComponentData>>> ListView;
	TMap<UStaticMeshComponent*, ECheckBoxState> EnableStates;
	int32 NumEnabledComponents;

	void Update();
	void UpdateState();
	void UpdateComponents();
};

class SGigaMergingDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGigaMergingDialog) {}
	SLATE_END_ARGS()

	SGigaMergingDialog();
	virtual ~SGigaMergingDialog() override;

	void Construct(const FArguments& InArgs, FGigaMergeTool* InTool);

	void Reset() { SelectionList.Update(); }
	TArray<UPrimitiveComponent*> GetSelectedComponents();
	int32 GetNumSelected() const { return SelectionList.NumEnabledComponents; }

private:
	FGigaMergeTool* Tool;

	FSelectionList SelectionList;
	TSharedPtr<IDetailsView> SettingsView;
	bool bRefreshListView;

	void CreateSettingsView();
	void OnSelectionChange(UObject*) { Reset(); }
	void OnMapChange(uint32) { Reset(); }
	void OnNewCurrentLevel() { Reset(); }
	TSharedRef<ITableRow> MakeComponentListItemWidget(TSharedPtr<FComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable);
};
