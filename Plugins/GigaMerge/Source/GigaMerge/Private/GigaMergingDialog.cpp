#include "GigaMergingDialog.h"

#include "GigaMergeTool.h"
#include "SlateOptMacros.h"
#include "Engine/Selection.h"

#define LOCTEXT_NAMESPACE "MergeProxyDialog"

void FSelectionList::Update()
{
	UpdateState();
	UpdateComponents();
	ListView->ClearSelection();
	ListView->RequestListRefresh();
}

void FSelectionList::UpdateState()
{
	EnableStates.Empty();

	for (auto ComponentData : SelectedComponents)
	{
		const ECheckBoxState State = ComponentData->bEnable ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		EnableStates.Add(ComponentData->Component.Get(), State);
	}
}

void FSelectionList::UpdateComponents()
{
	SelectedComponents.Empty();
	NumEnabledComponents = 0;

	USelection* SelectedActors = GEditor->GetSelectedActors();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		if (auto Actor = Cast<AActor>(*Iter))
		{
			Actors.Add(Actor);
			UniqueLevels.AddUnique(Actor->GetLevel());
		}
	}

	for (auto Actor : Actors)
	{
		check(Actor != nullptr);

		TArray<UChildActorComponent*> ChildActorComponents;
		Actor->GetComponents<UChildActorComponent>(ChildActorComponents);
		for (auto Child : ChildActorComponents)
		{
			if (auto ChildActor = Child->GetChildActor())
			{
				Actors.Add(ChildActor);
			}
		}

		TArray<UStaticMeshComponent*> MeshComponents;
		Actor->GetComponents<UStaticMeshComponent>(MeshComponents);
		for (auto Component : MeshComponents)
		{
			SelectedComponents.Add(MakeShared<FComponentData>(Component));
			auto& ComponentData = SelectedComponents.Last();
			ComponentData->bEnable = Component->GetStaticMesh() != nullptr;
			
			if (ECheckBoxState* State = EnableStates.Find(Component))
			{
				ComponentData->bEnable = *State == ECheckBoxState::Checked;
			}

			if (ComponentData->bEnable)
			{
				NumEnabledComponents++;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "SMeshMergingDialog" // temporarily use MergeActors LOCTEXT

SGigaMergingDialog::SGigaMergingDialog() : Tool(nullptr), SelectionList(), bRefreshListView(false) {}

SGigaMergingDialog::~SGigaMergingDialog()
{
	USelection::SelectionChangedEvent.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
	FEditorDelegates::MapChange.RemoveAll(this);
	FEditorDelegates::NewCurrentLevel.RemoveAll(this);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SGigaMergingDialog::Construct(const FArguments& InArgs, FGigaMergeTool* InTool)
{
	checkf(InTool != nullptr, TEXT("Invalid owner tool supplied"))
	Tool = InTool;

	SelectionList.UpdateComponents();
	CreateSettingsView();

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MergeStaticMeshComponentsLable", "Mesh Components to be incorporated in the merge:"))
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SAssignNew(SelectionList.ListView, SListView<TSharedPtr<FComponentData>>)
					.ListItemsSource(&SelectionList.SelectedComponents)
					.OnGenerateRow(this, &SGigaMergingDialog::MakeComponentListItemWidget)
					.ToolTipText(LOCTEXT("SelectedComponentsListBoxToolTip", "The selected mesh components will be incorporated into the merged mesh"))

				]
			]
		]
		+ SVerticalBox::Slot()
		.Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SettingsView->AsShared()
					]
				]
			]
		]
	];

	USelection::SelectionChangedEvent.AddSP(this, &SGigaMergingDialog::OnSelectionChange);
	USelection::SelectObjectEvent.AddSP(this, &SGigaMergingDialog::OnSelectionChange);
	FEditorDelegates::MapChange.AddSP(this, &SGigaMergingDialog::OnMapChange);
	FEditorDelegates::NewCurrentLevel.AddSP(this, &SGigaMergingDialog::OnNewCurrentLevel);

	SettingsView->SetObject(UGigaMergeToolSettings::Get());
}

TArray<UPrimitiveComponent*> SGigaMergingDialog::GetSelectedComponents()
{
	TArray<UPrimitiveComponent*> OutComponents;
	auto& SelectedComponents = SelectionList.SelectedComponents;
	for (auto ComponentData : SelectedComponents)
	{
		if (ComponentData.IsValid() && ComponentData->Component.IsValid() && ComponentData->Component->GetStaticMesh())
		{
			OutComponents.Add(ComponentData->Component.Get());
		}
	}
	return OutComponents;
}

void SGigaMergingDialog::CreateSettingsView()
{
	auto& EditorModule = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	DetailsViewArgs.bCustomNameAreaLocation = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.bCustomFilterAreaLocation = true;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

	SettingsView = EditorModule.CreateDetailView(DetailsViewArgs);
	SettingsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
		[](const FPropertyAndParent& PropertyAndParent)
		{
			return PropertyAndParent.Property.GetFName() != GET_MEMBER_NAME_CHECKED(FMaterialProxySettings, GutterSpace);
		}
	));
}

TSharedRef<ITableRow> SGigaMergingDialog::MakeComponentListItemWidget(TSharedPtr<FComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(ComponentData.IsValid());
	check(ComponentData->Component != nullptr);

	auto Component = ComponentData->Component;
	ECheckBoxState State = ComponentData->bEnable ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

	return SNew(STableRow<TSharedPtr<FComponentData>>, OwnerTable)
	[
		SNew(SBox)
		[
			SNew(SHorizontalBox)
			.IsEnabled(Component->GetStaticMesh() != nullptr)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.IsChecked(State)
				.ToolTipText(LOCTEXT("IncorporateCheckBoxToolTip", "When ticked the Component will be incorporated into the merge"))
				.OnCheckStateChanged_Lambda([ComponentData](ECheckBoxState NewState)
				{
					ComponentData->bEnable = NewState == ECheckBoxState::Checked;
				})
			]
			+ SHorizontalBox::Slot()
			  .Padding(5.0f, 0.0f, 0.0f, 0.0f)
			  .AutoWidth()
			[
				SNew(STextBlock)
				.Text_Lambda([Component]()
				{
					if (Component.IsValid())
					{
						const FString OwningActorName = Component->GetOwner()->GetName();
						const FString ComponentName = Component->GetName();
						const FString ComponentInfo = Component->GetStaticMesh() ? Component->GetStaticMesh()->GetName() : TEXT("None");

						return FText::FromString(OwningActorName + " - " + ComponentName + " - " + ComponentInfo);
					}
					return FText::FromString("Invalid Actor");
				})
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
