#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CircuitWidgets.generated.h"

class ACircuitWorkbench;
class AComponentBox;
class UTextBlock;
class UButton;
class UBorder;
class UCircuitWorkbenchWidget;

/** Focusable tile; click/Enter selects, mouse dragging carries the component actor. */
UCLASS()
class SCHOOL_API UCircuitTile : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UCircuitWorkbenchWidget> Panel;
    UPROPERTY() TObjectPtr<AComponentBox> Part;
    UPROPERTY() FName SlotName;
    void Refresh();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) override;
    virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
private:
    UPROPERTY() TObjectPtr<UTextBlock> Text;
    UPROPERTY() TObjectPtr<UBorder> Border;
    void ActivateTile();
};

UCLASS()
class SCHOOL_API UCircuitWorkbenchWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<ACircuitWorkbench> Bench;
    UPROPERTY() TObjectPtr<AComponentBox> SelectedPart;
    void SelectPart(AComponentBox* Part);
    bool PlaceSelected(FName TargetSlot);
    void Refresh();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UPROPERTY() TObjectPtr<UTextBlock> Status;
    UPROPERTY() TObjectPtr<UButton> RunButton;
    UPROPERTY() TObjectPtr<UButton> ResetButton;
    UPROPERTY() TArray<TObjectPtr<UCircuitTile>> Tiles;
    UFUNCTION() void RunTest();
    UFUNCTION() void ResetCircuit();
    UFUNCTION() void ClosePanel();
    UFUNCTION() void RetrySave();
};

UCLASS()
class SCHOOL_API UCircuitHUDWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<UTextBlock> Prompt;
    UPROPERTY() TObjectPtr<UTextBlock> Crosshair;
};
