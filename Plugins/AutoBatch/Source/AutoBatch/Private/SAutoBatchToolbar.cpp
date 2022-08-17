#include "SAutoBatchToolbar.h"

#include "LevelEditor.h"
#include "SlateOptMacros.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "SAutoBatchToolbar"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAutoBatchToolbar::Construct(const FArguments& InArgs)
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnActorSelectionChanged().AddRaw(this, &SAutoBatchToolbar::OnActorSelectionChanged);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .HAlign(HAlign_Left)
		  .Padding(2.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBoarder"))
			.Padding(0.0f)
			// .IsEnabled(this, &SAutoBatchToolbar::GetContentEnableState)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .FillHeight(1.0f)
				  .Padding(4.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(InlineContentHolder, SBox)
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Right)
				  .Padding(4.0f, 4.0f, 10.0f, 4.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Batch", "Auto Batch"))
					.OnClicked(this, &SAutoBatchToolbar::OnBatchClicked)
				]
			]
		]
	];

	GUnrealEd->UpdateFloatingPropertyWindows();
}

SAutoBatchToolbar::~SAutoBatchToolbar()
{
	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnActorSelectionChanged().RemoveAll(this);
}

void SAutoBatchToolbar::OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh)
{
	SelectedObjects = NewSelection;
}

FReply SAutoBatchToolbar::OnBatchClicked()
{
	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
