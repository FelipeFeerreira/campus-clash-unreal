#include "CircuitWidgets.h"
#include "CircuitActors.h"
#include "CircuitManager.h"
#include "StudentProgress.h"
#include "schoolPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/UniformGridPanel.h"
#include "Input/Reply.h"

namespace
{
    UTextBlock* Label(UWidgetTree* Tree, const FString& String, int32 Size = 18)
    {
        auto* Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(String));
        auto Font = Text->GetFont(); Font.Size = Size; Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor(.93,.95,.97)));
        Text->SetAutoWrapText(true);
        return Text;
    }
    UButton* Button(UWidgetTree* Tree, UHorizontalBox* Row, const FString& Text)
    {
        auto* B = Tree->ConstructWidget<UButton>();
        B->SetBackgroundColor(FLinearColor(.04,.28,.34));
        B->AddChild(Label(Tree, Text, 17));
        auto* Slot = Row->AddChildToHorizontalBox(B);
        Slot->SetPadding(FMargin(5,12));
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        return B;
    }
}

TSharedRef<SWidget> UCircuitTile::RebuildWidget()
{
    SetIsFocusable(true);
    auto* Size = WidgetTree->ConstructWidget<USizeBox>();
    WidgetTree->RootWidget = Size;
    Size->SetWidthOverride(SlotName.IsNone() ? 245 : 245);
    Size->SetHeightOverride(66);
    Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetPadding(FMargin(10));
    Size->AddChild(Border);
    Text = Label(WidgetTree,TEXT(""),16);
    Border->AddChild(Text);
    Refresh();
    return Super::RebuildWidget();
}

void UCircuitTile::Refresh()
{
    if (!Text || !Panel || !Panel->Bench) return;
    if (SlotName.IsNone() && Part)
    {
        Text->SetText(FText::FromString(Part->Data.DisplayName.ToString() + (Part->PlacedSlot.IsNone() ? TEXT("") : TEXT(" [encaixado]"))));
        Border->SetBrushColor(Panel->SelectedPart == Part ? FLinearColor(.05,.45,.5) : FLinearColor(.10,.17,.23));
    }
    else
    {
        const auto* Data = Panel->Bench->Circuit->Slots.Find(SlotName);
        const FString Name = SlotName == TEXT("Source") ? TEXT("1. FONTE") : SlotName == TEXT("Resistor") ? TEXT("2. RESISTOR") : TEXT("3. LAMPADA");
        Text->SetText(FText::FromString(Name + TEXT("\n") + (Data ? Data->DisplayName.ToString() : TEXT("Clique ou solte aqui"))));
        Border->SetBrushColor(Data ? FLinearColor(.06,.30,.25) : FLinearColor(.15,.20,.27));
    }
}

void UCircuitTile::ActivateTile()
{
    if (!Panel) return;
    if (SlotName.IsNone()) Panel->SelectPart(Part);
    else Panel->PlaceSelected(SlotName);
}

FReply UCircuitTile::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        ActivateTile();
        if (Part && Part->PlacedSlot.IsNone()) return UWidgetBlueprintLibrary::DetectDragIfPressed(Event, this, EKeys::LeftMouseButton).NativeReply;
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(Geometry, Event);
}

FReply UCircuitTile::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Enter || Event.GetKey() == EKeys::SpaceBar) { ActivateTile(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UCircuitTile::NativeOnDragDetected(const FGeometry&, const FPointerEvent&, UDragDropOperation*& Operation)
{
    if (!Part || !Panel || !Panel->Bench || Panel->Bench->Circuit->Phase != ECircuitPhase::Assembly || !Part->PlacedSlot.IsNone()) return;
    Operation = NewObject<UDragDropOperation>(this);
    Operation->Payload = Part;
    auto* Visual = CreateWidget<UCircuitTile>(GetOwningPlayer());
    Visual->Panel = Panel; Visual->Part = Part;
    Operation->DefaultDragVisual = Visual;
    Operation->Pivot = EDragPivot::MouseDown;
}

bool UCircuitTile::NativeOnDrop(const FGeometry&, const FDragDropEvent&, UDragDropOperation* Operation)
{
    if (SlotName.IsNone() || !Panel || !Operation) return false;
    auto* Dragged = Cast<AComponentBox>(Operation->Payload);
    if (!Dragged || Dragged->Bench != Panel->Bench) return false;
    Panel->SelectedPart = Dragged;
    return Panel->PlaceSelected(SlotName);
}

TSharedRef<SWidget> UCircuitWorkbenchWidget::RebuildWidget()
{
    Tiles.Reset();
    SetIsFocusable(true);
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    auto* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(.015,.028,.048,.98));
    Border->SetPadding(FMargin(25));
    auto* Placement = Canvas->AddChildToCanvas(Border);
    Placement->SetAnchors(FAnchors(.5,.5)); Placement->SetAlignment(FVector2D(.5,.5));
    // Altura comporta o texto de falha (4 linhas) sem empurrar os botões para fora do painel.
    Placement->SetSize(FVector2D(850,700));
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Border->AddChild(Column);
    Column->AddChildToVerticalBox(Label(WidgetTree,TEXT("TECHRIDGE  /  CIRCUITOS I"),28));
    Column->AddChildToVerticalBox(Label(WidgetTree,TEXT("Monte um circuito em serie. Alvo: 0,9 - 1,1 A\nI = V / (R do resistor + R da lampada). A bancada conecta os fios."),17));
    Column->AddChildToVerticalBox(Label(WidgetTree,TEXT("CATALOGO - arraste uma peca ou selecione e clique no encaixe"),16))->SetPadding(FMargin(0,14,0,8));
    auto* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    Grid->SetSlotPadding(FMargin(4)); Column->AddChildToVerticalBox(Grid);
    if (Bench)
    {
        int32 Index = 0;
        for (AComponentBox* Part : Bench->Components)
        {
            auto* Tile = CreateWidget<UCircuitTile>(GetOwningPlayer());
            Tile->Panel = this; Tile->Part = Part; Tiles.Add(Tile);
            Grid->AddChildToUniformGrid(Tile, Index / 3, Index % 3); ++Index;
        }
    }
    Column->AddChildToVerticalBox(Label(WidgetTree,TEXT("ENCAIXES - clique sem selecao para retirar uma peca"),16))->SetPadding(FMargin(0,14,0,8));
    auto* Sockets = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Sockets);
    for (FName Name : {FName(TEXT("Source")),FName(TEXT("Resistor")),FName(TEXT("Bulb"))})
    {
        auto* Tile = CreateWidget<UCircuitTile>(GetOwningPlayer());
        Tile->Panel = this; Tile->SlotName = Name; Tiles.Add(Tile);
        Sockets->AddChildToHorizontalBox(Tile)->SetPadding(FMargin(4));
    }
    Status = Label(WidgetTree,TEXT(""),18);
    Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(0,14));
    auto* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChildToVerticalBox(Buttons);
    RunButton = Button(WidgetTree, Buttons, TEXT("Testar circuito")); RunButton->OnClicked.AddDynamic(this,&UCircuitWorkbenchWidget::RunTest);
    ResetButton = Button(WidgetTree, Buttons, TEXT("Reiniciar")); ResetButton->OnClicked.AddDynamic(this,&UCircuitWorkbenchWidget::ResetCircuit);
    Button(WidgetTree, Buttons, TEXT("Salvar nota"))->OnClicked.AddDynamic(this,&UCircuitWorkbenchWidget::RetrySave);
    Button(WidgetTree, Buttons, TEXT("Fechar [Esc]"))->OnClicked.AddDynamic(this,&UCircuitWorkbenchWidget::ClosePanel);
    Refresh();
    return Super::RebuildWidget();
}

void UCircuitWorkbenchWidget::SelectPart(AComponentBox* Part)
{
    if (!Bench || Bench->Circuit->Phase != ECircuitPhase::Assembly || !Part || !Part->PlacedSlot.IsNone()) return;
    SelectedPart = SelectedPart == Part ? nullptr : Part;
    Refresh();
}

bool UCircuitWorkbenchWidget::PlaceSelected(FName TargetSlot)
{
    if (!Bench) return false;
    const bool Success = SelectedPart ? Bench->AddComponentToSlot(SelectedPart,TargetSlot) : Bench->RemoveFromSlot(TargetSlot);
    if (Success) SelectedPart = nullptr;
    else if (SelectedPart) Bench->Circuit->LastResult.Message = FText::FromString(TEXT("Encaixe incompativel: fonte, resistor e lampada possuem lugares proprios."));
    Refresh();
    return Success;
}

void UCircuitWorkbenchWidget::Refresh()
{
    if (!Status || !Bench) return;
    auto* Manager = Bench->Circuit.Get();
    FString Message = Manager->LastResult.Message.IsEmpty() ? TEXT("Escolha as tres pecas. A lampada adiciona 1 ohm ao circuito.") : Manager->LastResult.Message.ToString();
    if (Manager->LastResult.bValid) Message += FString::Printf(TEXT("\nV: %.1f V   R: %.1f ohm   I: %.3f A   Nota: %.0f/100"),Manager->LastResult.Voltage,Manager->LastResult.Resistance,Manager->LastResult.Current,Manager->LastResult.Grade);
    if (Manager->Phase == ECircuitPhase::Burning) Message += FString::Printf(TEXT("\nFocos ativos: %d | E: pegar extintor | Segure F e mire com +"),Bench->Fires.Num());
    if (Bench->bSaveWarning) Message += TEXT("\nNota em memoria: falha ao salvar. Use Salvar nota.");
    Status->SetText(FText::FromString(Message));
    Status->SetColorAndOpacity(FSlateColor(Manager->Phase == ECircuitPhase::Burning ? FLinearColor(1,.35,.22) : FLinearColor(.85,.96,1)));
    RunButton->SetIsEnabled(Manager->CanRunTest());
    ResetButton->SetIsEnabled(Manager->Phase != ECircuitPhase::Burning);
    for (UCircuitTile* Tile : Tiles) Tile->Refresh();
}

void UCircuitWorkbenchWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds) { Super::NativeTick(Geometry,DeltaSeconds); Refresh(); }
void UCircuitWorkbenchWidget::RunTest() { if (Bench) Bench->RunTest(); Refresh(); }
void UCircuitWorkbenchWidget::ResetCircuit() { if (Bench) Bench->ResetCircuit(); SelectedPart=nullptr; Refresh(); }
void UCircuitWorkbenchWidget::ClosePanel() { if (auto* PC=Cast<AschoolPlayerController>(GetOwningPlayer())) PC->CloseCircuitPanel(); }
void UCircuitWorkbenchWidget::RetrySave()
{
    if (auto* Progress=GetGameInstance()->GetSubsystem<UStudentProgress>()) if(Bench) Bench->bSaveWarning=!Progress->SaveProgress();
    Refresh();
}
FReply UCircuitWorkbenchWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if (Event.GetKey()==EKeys::Escape) { ClosePanel(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry,Event);
}

TSharedRef<SWidget> UCircuitHUDWidget::RebuildWidget()
{
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget=Canvas;
    auto* Border=WidgetTree->ConstructWidget<UBorder>(); Border->SetBrushColor(FLinearColor(.015,.03,.045,.85)); Border->SetPadding(FMargin(16));
    auto* PromptSlot=Canvas->AddChildToCanvas(Border); PromptSlot->SetPosition(FVector2D(24,24)); PromptSlot->SetSize(FVector2D(540,195));
    Prompt=Label(WidgetTree,TEXT(""),18); Border->AddChild(Prompt);
    Crosshair=Label(WidgetTree,TEXT("+"),26);
    auto* CrossSlot=Canvas->AddChildToCanvas(Crosshair); CrossSlot->SetAnchors(FAnchors(.5,.5)); CrossSlot->SetAlignment(FVector2D(.5,.5)); CrossSlot->SetSize(FVector2D(30,40));
    SetVisibility(ESlateVisibility::HitTestInvisible);
    return Super::RebuildWidget();
}

void UCircuitHUDWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    auto* PC=Cast<AschoolPlayerController>(GetOwningPlayer());
    if (!PC || !Prompt) return;
    Prompt->SetText(FText::FromString(PC->GetCircuitPrompt()));
    Crosshair->SetColorAndOpacity(FSlateColor(PC->IsExtinguisherHittingFire() ? FLinearColor(.1,1,.5) : FLinearColor::White));
}
