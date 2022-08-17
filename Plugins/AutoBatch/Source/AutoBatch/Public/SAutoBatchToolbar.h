#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AUTOBATCH_API SAutoBatchToolbar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAutoBatchToolbar) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual ~SAutoBatchToolbar() override;

private:
	TArray<UObject*> SelectedObjects;

	TSharedPtr<SBox> InlineContentHolder;
	
	void OnActorSelectionChanged(const TArray<UObject*>& NewSelection, bool bForceRefresh);
	FReply OnBatchClicked();
};
