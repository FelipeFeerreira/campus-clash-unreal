#include "CalculusWidgets.h"
#include "CalculusClassroom.h"
#include "CalculusLesson.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    // Mesmas cores do painel de circuitos, para a UI do jogo ficar consistente.
    const FLinearColor PanelColor(.015f, .028f, .048f, .96f);
    const FLinearColor ChoiceColor(.20f, .27f, .33f);
    const FLinearColor ChoiceHover(.07f, .36f, .42f);
    const FLinearColor ChoicePicked(.20f, .56f, .45f);
    const FLinearColor TextColor(.93f, .95f, .97f);

    UTextBlock* Label(UWidgetTree* Tree, int32 Size = 18)
    {
        auto* Text = Tree->ConstructWidget<UTextBlock>();
        auto Font = Text->GetFont(); Font.Size = Size; Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(TextColor));
        Text->SetAutoWrapText(true);
        return Text;
    }

    UButton* Button(UWidgetTree* Tree, UHorizontalBox* Row, const TCHAR* Caption)
    {
        auto* B = Tree->ConstructWidget<UButton>();
        B->SetBackgroundColor(FLinearColor(.04f, .28f, .34f));
        auto* Text = Label(Tree, 17); Text->SetText(FText::FromString(Caption));
        B->AddChild(Text);
        auto* Slot = Row->AddChildToHorizontalBox(B);
        Slot->SetPadding(FMargin(5, 8));
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        return B;
    }

    /** Tecla numérica (linha de cima ou teclado numérico) -> índice 0-8, ou -1. */
    int32 DigitIndex(const FKey& Key)
    {
        static const FKey Top[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
        static const FKey Pad[] = { EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour, EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
        for (int32 i = 0; i < 9; ++i) if (Key == Top[i] || Key == Pad[i]) return i;
        return -1;
    }
}

// ---------------- UCalcChoice ----------------

TSharedRef<SWidget> UCalcChoice::RebuildWidget()
{
    auto* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetMinDesiredHeight(46);
    WidgetTree->RootWidget = Size;
    Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetPadding(FMargin(12, 8));
    Size->AddChild(Border);
    auto* Text = Label(WidgetTree, 18);
    Text->SetText(FText::Format(FText::FromString(TEXT("{0})  {1}")), FText::AsNumber(Index + 1), Caption));
    Border->AddChild(Text);
    Paint(false);
    return Super::RebuildWidget();
}

void UCalcChoice::Paint(bool bHover)
{
    if (Border) Border->SetBrushColor(bPicked ? ChoicePicked : bHover ? ChoiceHover : ChoiceColor);
}

FReply UCalcChoice::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Panel && !bPicked)
    {
        Panel->Choose(Index);
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(Geometry, Event);
}

void UCalcChoice::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) { Super::NativeOnMouseEnter(Geometry, Event); Paint(true); }
void UCalcChoice::NativeOnMouseLeave(const FPointerEvent& Event) { Super::NativeOnMouseLeave(Event); Paint(false); }

// ---------------- UCalculusWidget ----------------

TSharedRef<SWidget> UCalculusWidget::RebuildWidget()
{
    SetIsFocusable(true);
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;

    // Caixa de fala do professor (rodapé), usada na introdução.
    DialogBox = WidgetTree->ConstructWidget<UBorder>();
    DialogBox->SetBrushColor(PanelColor);
    DialogBox->SetPadding(FMargin(24, 16));
    auto* DialogSlot = Canvas->AddChildToCanvas(DialogBox);
    DialogSlot->SetAnchors(FAnchors(.5f, 1.f));
    DialogSlot->SetAlignment(FVector2D(.5f, 1.f));
    DialogSlot->SetPosition(FVector2D(0, -30));
    DialogSlot->SetSize(FVector2D(1000, 190));
    auto* DialogColumn = WidgetTree->ConstructWidget<UVerticalBox>();
    DialogBox->AddChild(DialogColumn);
    DialogName = Label(WidgetTree, 20);
    DialogName->SetColorAndOpacity(FSlateColor(FLinearColor(.45f, .85f, .95f)));
    DialogColumn->AddChildToVerticalBox(DialogName);
    DialogText = Label(WidgetTree, 22);
    DialogColumn->AddChildToVerticalBox(DialogText)->SetPadding(FMargin(0, 8));
    auto* Hint = Label(WidgetTree, 15);
    Hint->SetText(FText::FromString(TEXT("[E] ou clique para continuar")));
    Hint->SetColorAndOpacity(FSlateColor(FLinearColor(.6f, .66f, .72f)));
    DialogColumn->AddChildToVerticalBox(Hint);

    // Painel central: dificuldade, perguntas, quadro e resultado.
    MainPanel = WidgetTree->ConstructWidget<UBorder>();
    MainPanel->SetBrushColor(PanelColor);
    MainPanel->SetPadding(FMargin(26));
    auto* MainSlot = Canvas->AddChildToCanvas(MainPanel);
    MainSlot->SetAnchors(FAnchors(.5f, .5f));
    MainSlot->SetAlignment(FVector2D(.5f, .5f));
    MainSlot->SetSize(FVector2D(900, 720));
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    MainPanel->AddChild(Column);
    Title = Label(WidgetTree, 28);
    Column->AddChildToVerticalBox(Title);
    Info = Label(WidgetTree, 17);
    Info->SetColorAndOpacity(FSlateColor(FLinearColor(.45f, .85f, .95f)));
    Column->AddChildToVerticalBox(Info)->SetPadding(FMargin(0, 6));
    Body = Label(WidgetTree, 22);
    Column->AddChildToVerticalBox(Body)->SetPadding(FMargin(0, 12));
    Choices = WidgetTree->ConstructWidget<UVerticalBox>();
    Column->AddChildToVerticalBox(Choices);
    FeedbackText = Label(WidgetTree, 18);
    Column->AddChildToVerticalBox(FeedbackText)->SetPadding(FMargin(0, 12));
    auto* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Buttons);
    ClearButton = Button(WidgetTree, Buttons, TEXT("Limpar ordem [Backspace]"));
    ClearButton->OnClicked.AddDynamic(this, &UCalculusWidget::OnClear);
    RetryButton = Button(WidgetTree, Buttons, TEXT("Repetir"));
    RetryButton->OnClicked.AddDynamic(this, &UCalculusWidget::OnRetry);
    CampusButton = Button(WidgetTree, Buttons, TEXT("Voltar ao campus"));
    CampusButton->OnClicked.AddDynamic(this, &UCalculusWidget::OnCampus);
    return Super::RebuildWidget();
}

void UCalculusWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Room && Room->Lesson) Room->Lesson->OnChanged.AddUniqueDynamic(this, &UCalculusWidget::HandleLessonChanged);
    Refresh();
}

void UCalculusWidget::HandleLessonChanged()
{
    // Nova pergunta/tentativa: a seleção do quadro recomeça.
    BoardPicked.Reset();
    MarkScreenChange();
    Refresh();
}

void UCalculusWidget::MarkScreenChange()
{
    if (const UWorld* World = GetWorld()) LastScreenChange = World->GetRealTimeSeconds();
}

bool UCalculusWidget::IsInputReady() const
{
    const UWorld* World = GetWorld();
    return !World || World->GetRealTimeSeconds() - LastScreenChange >= 0.3;
}

void UCalculusWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    Refresh(); // o timer muda a cada frame
}

void UCalculusWidget::RebuildChoices(const TArray<FText>& Captions)
{
    Choices->ClearChildren();
    for (int32 i = 0; i < Captions.Num(); ++i)
    {
        auto* Choice = CreateWidget<UCalcChoice>(this);
        Choice->Panel = this;
        Choice->Index = i;
        Choice->Caption = Captions[i];
        Choice->bPicked = BoardPicked.Contains(i);
        Choices->AddChildToVerticalBox(Choice)->SetPadding(FMargin(0, 4));
    }
}

void UCalculusWidget::Refresh()
{
    if (!Room || !Room->Lesson || !MainPanel) return;
    const UCalculusLesson* L = Room->Lesson;
    const ECalcPhase Phase = L->Phase;

    DialogBox->SetVisibility(Phase == ECalcPhase::Intro ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    MainPanel->SetVisibility(Phase == ECalcPhase::Intro || Phase == ECalcPhase::Idle ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    ClearButton->SetVisibility(Phase == ECalcPhase::Board ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    RetryButton->SetVisibility(Phase == ECalcPhase::Result ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    CampusButton->SetVisibility(Phase == ECalcPhase::Result || Phase == ECalcPhase::ChooseDifficulty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    FeedbackText->SetText(L->Feedback);
    FeedbackText->SetColorAndOpacity(FSlateColor(L->bLastWasCorrect ? FLinearColor(.35f, 1.f, .55f) : FLinearColor(1.f, .45f, .35f)));

    TArray<FText> Captions;
    FString Signature = FString::Printf(TEXT("%d"), (int32)Phase);
    switch (Phase)
    {
    case ECalcPhase::Intro:
        DialogName->SetText(Room->ProfessorName);
        DialogText->SetText(Room->GetIntroLine());
        break;
    case ECalcPhase::ChooseDifficulty:
        Title->SetText(FText::FromString(TEXT("CÁLCULO I")));
        Info->SetText(FText::FromString(TEXT("Escolha a dificuldade (clique ou teclas 1-3)")));
        Body->SetText(FText::GetEmpty());
        for (ECalcDifficulty D : { ECalcDifficulty::Easy, ECalcDifficulty::Medium, ECalcDifficulty::Hard })
            Captions.Add(FText::FromString(FString::Printf(TEXT("%s   (melhor nota: %.0f)"), *UCalculusLesson::DifficultyName(D).ToString(), Room->GetBestGrade(D))));
        break;
    case ECalcPhase::Question:
    {
        const FCalcQuestionRow& Q = L->LessonQuestions[L->QuestionIndex];
        Title->SetText(FText::FromString(FString::Printf(TEXT("Pergunta %d de %d"), L->QuestionIndex + 1, L->LessonQuestions.Num())));
        Info->SetText(FText::FromString(FString::Printf(TEXT("%s   |   Tempo: %.0f s   |   Pontos: %d"), *UCalculusLesson::DifficultyName(L->Difficulty).ToString(), FMath::CeilToFloat(L->TimeLeft), L->Points)));
        Body->SetText(Q.Question);
        Captions = L->Options;
        Signature += FString::Printf(TEXT("-%d"), L->QuestionIndex);
        break;
    }
    case ECalcPhase::Board:
        Title->SetText(FText::FromString(TEXT("Desafio do quadro")));
        Info->SetText(FText::FromString(FString::Printf(TEXT("Clique nos passos na ordem certa   |   Tempo: %.0f s   |   Pontos: %d"), FMath::CeilToFloat(L->TimeLeft), L->Points)));
        {
            // Mostra a ordem que o jogador está montando.
            FString Order;
            for (int32 i = 0; i < BoardPicked.Num(); ++i) Order += FString::Printf(TEXT("\n%d. %s"), i + 1, *L->BoardSteps[BoardPicked[i]].ToString());
            Body->SetText(FText::FromString(L->Board.Problem.ToString() + (Order.IsEmpty() ? TEXT("") : TEXT("\n\nSua ordem:") + Order)));
        }
        Captions = L->BoardSteps;
        Signature += FString::Printf(TEXT("-%d"), BoardPicked.Num());
        break;
    case ECalcPhase::Result:
    {
        Title->SetText(FText::FromString(TEXT("Resultado")));
        Info->SetText(UCalculusLesson::DifficultyName(L->Difficulty));
        FString Text = FString::Printf(TEXT("Nota: %.0f / 100%s\nAcertos: %d de %d\nQuadro: %s\nPontos: %d de %d\nMelhor nota nesta dificuldade: %.0f"),
            L->FinalGrade, L->FinalGrade >= 60.f ? TEXT("  (aprovado)") : TEXT("  (reprovado)"),
            L->CorrectAnswers, L->LessonQuestions.Num(),
            !L->bHasBoard ? TEXT("sem desafio") : L->bBoardSolved ? TEXT("resolvido") : TEXT("não resolvido"),
            FMath::Max(0, L->Points), L->GetMaxPoints(), Room->GetBestGrade(L->Difficulty));
        if (Room->bSaveFailed) Text += TEXT("\nAviso: falha ao salvar a nota.");
        Body->SetText(FText::FromString(Text));
        break;
    }
    default:
        break;
    }
    if (Signature != ChoicesSignature)
    {
        ChoicesSignature = Signature;
        RebuildChoices(Captions);
    }
}

void UCalculusWidget::Choose(int32 Index)
{
    if (!Room || !Room->Lesson || !IsInputReady()) return;
    UCalculusLesson* L = Room->Lesson;
    switch (L->Phase)
    {
    case ECalcPhase::ChooseDifficulty:
        if (Index >= 0 && Index <= 2) L->ChooseDifficulty(static_cast<ECalcDifficulty>(Index));
        break;
    case ECalcPhase::Question:
        L->AnswerQuestion(Index);
        break;
    case ECalcPhase::Board:
        if (!L->BoardSteps.IsValidIndex(Index) || BoardPicked.Contains(Index)) return;
        BoardPicked.Add(Index);
        // Ao escolher todos os passos, envia; se errar, a seleção é limpa (HandleLessonChanged).
        if (BoardPicked.Num() == L->BoardSteps.Num()) L->SubmitBoardOrder(TArray<int32>(BoardPicked));
        Refresh();
        break;
    default:
        break;
    }
}

FReply UCalculusWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!Room || !Room->Lesson) return Super::NativeOnKeyDown(Geometry, Event);
    const FKey Key = Event.GetKey();
    // Tecla segurada (repetição) não conta como nova resposta.
    if (Event.IsRepeat() || !IsInputReady()) return FReply::Handled();
    const ECalcPhase Phase = Room->Lesson->Phase;
    if (Phase == ECalcPhase::Intro && (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar))
    {
        Room->AdvanceIntro();
        MarkScreenChange();
        return FReply::Handled();
    }
    if (Phase == ECalcPhase::Board && Key == EKeys::BackSpace) { OnClear(); return FReply::Handled(); }
    const int32 Digit = DigitIndex(Key);
    if (Digit >= 0) { Choose(Digit); return FReply::Handled(); }
    return FReply::Handled(); // modo só-UI: nenhuma tecla deve vazar para o jogo
}

FReply UCalculusWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    // Clique em qualquer lugar avança a fala do professor.
    if (Room && Room->Lesson && Room->Lesson->Phase == ECalcPhase::Intro && Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (IsInputReady()) { Room->AdvanceIntro(); MarkScreenChange(); }
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(Geometry, Event);
}

void UCalculusWidget::OnRetry() { if (Room && Room->Lesson && IsInputReady()) Room->Lesson->Retry(); }
void UCalculusWidget::OnCampus() { if (Room && IsInputReady()) Room->LeaveToCampus(); }
void UCalculusWidget::OnClear() { BoardPicked.Reset(); Refresh(); }
