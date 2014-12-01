#include "LogVisualizer.h"
#include "STimelinesContainer.h"
#include "STimeline.h"
#include "STimelineBar.h"
#include "TimeSliderController.h"

TSharedRef<SWidget> STimelinesContainer::MakeTimeline(TSharedPtr<class SVisualLoggerView> VisualLoggerView, TSharedPtr<class FSequencerTimeSliderController> TimeSliderController, const FVisualLogDevice::FVisualLogEntryItem& Entry)
{
	TSharedPtr<STimeline> NewTimeline;

	ContainingBorder->AddSlot()
		[
			SAssignNew(NewTimeline, STimeline, VisualLoggerView, TimeSliderController, SharedThis(this), Entry)
			.OnItemSelectionChanged(this->VisualLoggerInterface->GetVisualLoggerEvents().OnItemSelectionChanged)
			.VisualLoggerInterface(this->VisualLoggerInterface)
		];

	TimelineItems.Add(NewTimeline.ToSharedRef());

	return NewTimeline.ToSharedRef();
}

void STimelinesContainer::SetSelectionState(TSharedPtr<class STimeline> AffectedNode, bool bSelect, bool bDeselectOtherNodes)
{
	if (bSelect)
	{
		if (bDeselectOtherNodes)
		{
			// empty current selection set unless multiple selecting
			SelectedNodes.Empty();
		}

		SelectedNodes.Add(AffectedNode);
		AffectedNode->OnSelect();
		VisualLoggerInterface->GetVisualLoggerEvents().OnObjectSelectionChanged.ExecuteIfBound(AffectedNode);
	}
	else
	{
		// Not selecting so remove the node from the selection set
		SelectedNodes.Remove(AffectedNode);
		AffectedNode->OnDeselect();
	}
}

bool STimelinesContainer::IsNodeSelected(TSharedPtr<class STimeline> Node) const
{
	return SelectedNodes.Contains(Node);
}

void STimelinesContainer::ChangeSelection(class TSharedPtr<class STimeline> Timeline, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsLeftShiftDown() == false)
	{
		if (MouseEvent.IsLeftControlDown())
		{
			SetSelectionState(Timeline, !Timeline->IsSelected(), false);
		}
		else
		{
			SetSelectionState(Timeline, true, true);
		}
	}
	else
	{
		TSharedPtr<class STimeline> LastSelected = SelectedNodes[SelectedNodes.Num() - 1];
		bool bStartedSelection = false;
		for (auto& CurrentItem : TimelineItems)
		{
			if (CurrentItem == LastSelected || CurrentItem == Timeline)
			{
				if (!bStartedSelection)
				{
					bStartedSelection = true;
				}
				else
				{
					bStartedSelection = false;
					break;
				}
			}
			if (bStartedSelection)
			{
				SetSelectionState(CurrentItem, true, false);
			}
		}
		SetSelectionState(Timeline, true, false);
	}
}

FReply STimelinesContainer::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return TimeSliderController->OnMouseButtonDown(SharedThis(this), MyGeometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply STimelinesContainer::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return TimeSliderController->OnMouseButtonUp(SharedThis(this), MyGeometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply STimelinesContainer::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return TimeSliderController->OnMouseMove(SharedThis(this), MyGeometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply STimelinesContainer::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsLeftControlDown() || MouseEvent.IsLeftShiftDown())
	{
		return TimeSliderController->OnMouseWheel(SharedThis(this), MyGeometry, MouseEvent);
	}
	return FReply::Unhandled();
}

FReply STimelinesContainer::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::A && InKeyEvent.IsLeftControlDown())
	{
		for (auto& CurrentTimeline : TimelineItems)
		{
			SetSelectionState(CurrentTimeline, true, false);
		}

		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Platform_Delete && SelectedNodes.Num() > 0)
	{
		TWeakPtr<class STimeline>  NotSelectedOne;
		for (auto & CurrentNode : SelectedNodes)
		{
			TSharedPtr<class STimeline> LastSelected = SelectedNodes[SelectedNodes.Num() - 1];
			bool bFoundSelectedOne = false;
			for (auto& CurrentItem : TimelineItems)
			{
				if (IsNodeSelected(CurrentItem) == false)
				{
					NotSelectedOne = CurrentItem;
				}
				if (LastSelected == CurrentItem)
				{
					if (bFoundSelectedOne && NotSelectedOne.IsValid())
					{
						break;
					}
					bFoundSelectedOne = true;
				}
			}
			TimelineItems.Remove(CurrentNode);
			ContainingBorder->RemoveSlot(CurrentNode.ToSharedRef());
		}

		if (NotSelectedOne.IsValid())
		{
			SetSelectionState(NotSelectedOne.Pin(), true, true);
		}
		else
		{
			SelectedNodes.Reset();
		}
		return FReply::Handled();
	}
	else if (InKeyEvent.GetKey() == EKeys::Up || InKeyEvent.GetKey() == EKeys::Down)
	{
		TSharedPtr<class STimeline> PreviousTimeline;
		TSharedPtr<class STimeline> LastSelected = SelectedNodes[SelectedNodes.Num() - 1];
		for (int32 Index = 0; Index < TimelineItems.Num(); ++Index)
		{
			auto& CurrentItem = TimelineItems[Index];
			if (LastSelected == CurrentItem)
			{
				if (InKeyEvent.GetKey() == EKeys::Up && PreviousTimeline.IsValid())
				{
					SetSelectionState(PreviousTimeline, true, true);
				}
				else if (InKeyEvent.GetKey() == EKeys::Down)
				{
					// let's find next visible time line
					if (TimelineItems.IsValidIndex(Index + 1))
					{
						for (int32 i = Index + 1; i < TimelineItems.Num(); ++i)
						{
							if (TimelineItems[i]->GetVisibility() == EVisibility::Visible)
							{
								SetSelectionState(TimelineItems[i], true, true);
								break;
							}
						}
					}
				}
				break;
			}

			if (CurrentItem->GetVisibility() == EVisibility::Visible)
			{
				PreviousTimeline = CurrentItem;
			}
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void STimelinesContainer::Construct(const FArguments& InArgs, TSharedRef<class SVisualLoggerView> InVisualLoggerView, TSharedRef<FSequencerTimeSliderController> InTimeSliderController)
{
	TimeSliderController = InTimeSliderController;
	VisualLoggerView = InVisualLoggerView;
	VisualLoggerInterface = InArgs._VisualLoggerInterface.Get();

	ChildSlot
		[
			SNew(SBorder)
			.Padding(0)
			.VAlign(VAlign_Top)
			.BorderImage(FLogVisualizerStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(ContainingBorder, SVerticalBox)
			]
		];
}

void STimelinesContainer::OnSearchChanged(const FText& Filter)
{
	for (auto CurrentItem : TimelineItems)
	{
		CurrentItem->OnSearchChanged(Filter);
	}
}

void STimelinesContainer::OnNewLogEntry(const FVisualLogDevice::FVisualLogEntryItem& Entry)
{
	bool bCreateNew = true;
	for (auto CurrentItem : TimelineItems)
	{
		if (CurrentItem->GetName() == Entry.OwnerName)
		{
			CurrentItem->AddEntry(Entry);
			bCreateNew = false;
			break;
		}
	}

	if (bCreateNew)
	{
		MakeTimeline(VisualLoggerView, TimeSliderController, Entry);
	}

	TRange<float> LocalViewRange = TimeSliderController->GetTimeSliderArgs().ViewRange.Get();
	const float CurrentMin = TimeSliderController->GetTimeSliderArgs().ClampMin.Get().GetValue();
	const float CurrentMax = TimeSliderController->GetTimeSliderArgs().ClampMax.Get().GetValue();
	float ZoomLevel = LocalViewRange.Size<float>() / (CurrentMax - CurrentMin);

	TimeSliderController->GetTimeSliderArgs().ClampMin = FMath::Min(0.0f, CurrentMin);
	TimeSliderController->GetTimeSliderArgs().ClampMax = FMath::Max(Entry.Entry.TimeStamp, CurrentMax);
	if ( FMath::Abs(ZoomLevel - 1) <= SMALL_NUMBER)
	{
		TimeSliderController->GetTimeSliderArgs().ViewRange = TRange<float>(TimeSliderController->GetTimeSliderArgs().ClampMin.Get().GetValue(), TimeSliderController->GetTimeSliderArgs().ClampMax.Get().GetValue());
	}
}

void STimelinesContainer::OnFiltersChanged()
{
	for (auto CurrentItem : TimelineItems)
	{
		CurrentItem->OnFiltersChanged();
	}
}
