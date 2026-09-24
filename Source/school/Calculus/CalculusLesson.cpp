#include "CalculusLesson.h"

namespace
{
    /** Embaralhamento Fisher-Yates com o gerador da aula (reprodutível nos testes). */
    template <typename T>
    void Shuffle(TArray<T>& Items, FRandomStream& Random)
    {
        for (int32 i = Items.Num() - 1; i > 0; --i)
            Items.Swap(i, Random.RandRange(0, i));
    }
}

void UCalculusLesson::SetContent(const TArray<FCalcQuestionRow>& InQuestions, const TArray<FCalcBoardRow>& InBoards)
{
    AllQuestions = InQuestions;
    AllBoards = InBoards;
}

bool UCalculusLesson::StartLesson()
{
    if (Phase != ECalcPhase::Idle) return false;
    Phase = ECalcPhase::Intro;
    Changed();
    return true;
}

bool UCalculusLesson::FinishIntro()
{
    if (Phase != ECalcPhase::Intro) return false;
    Phase = ECalcPhase::ChooseDifficulty;
    Changed();
    return true;
}

bool UCalculusLesson::ChooseDifficulty(ECalcDifficulty InDifficulty)
{
    if (Phase != ECalcPhase::ChooseDifficulty) return false;
    Difficulty = InDifficulty;
    Points = CorrectAnswers = QuestionIndex = 0;
    FinalGrade = 0.f;
    bBoardSolved = false;
    Feedback = FText::GetEmpty();

    // Sorteia até QuestionsPerLesson perguntas da dificuldade escolhida, sem repetir.
    LessonQuestions.Reset();
    for (const FCalcQuestionRow& Row : AllQuestions)
        if (Row.Difficulty == Difficulty) LessonQuestions.Add(Row);
    Shuffle(LessonQuestions, Random);
    if (LessonQuestions.Num() > QuestionsPerLesson) LessonQuestions.SetNum(QuestionsPerLesson);

    // Um desafio de quadro da mesma dificuldade, se existir.
    TArray<const FCalcBoardRow*> Boards;
    for (const FCalcBoardRow& Row : AllBoards)
        if (Row.Difficulty == Difficulty && Row.GetSteps().Num() >= 2) Boards.Add(&Row);
    bHasBoard = Boards.Num() > 0;
    if (bHasBoard) Board = *Boards[Random.RandRange(0, Boards.Num() - 1)];

    if (LessonQuestions.Num() > 0)
    {
        Phase = ECalcPhase::Question;
        PrepareQuestion();
    }
    else if (bHasBoard) StartBoard();
    else FinishLesson();
    Changed();
    return true;
}

void UCalculusLesson::PrepareQuestion()
{
    const FCalcQuestionRow& Q = LessonQuestions[QuestionIndex];
    Options.Reset();
    Options.Add(Q.Correct);
    for (const FText* Wrong : { &Q.Wrong1, &Q.Wrong2, &Q.Wrong3 })
        if (!Wrong->IsEmptyOrWhitespace()) Options.Add(*Wrong);
    Shuffle(Options, Random);
    CorrectOption = Options.IndexOfByPredicate([&Q](const FText& T) { return T.EqualTo(Q.Correct); });
    TimeLeft = GetQuestionTime();
}

bool UCalculusLesson::AnswerQuestion(int32 OptionIndex)
{
    if (Phase != ECalcPhase::Question || !Options.IsValidIndex(OptionIndex)) return false;
    bLastWasCorrect = OptionIndex == CorrectOption;
    if (bLastWasCorrect)
    {
        Points += PointsPerCorrect;
        ++CorrectAnswers;
        Feedback = FText::FromString(FString::Printf(TEXT("Certo! +%d"), PointsPerCorrect));
    }
    else
    {
        ApplyWrong(FText::FromString(FString::Printf(TEXT("Errado. Resposta: %s  (-%d)"), *LessonQuestions[QuestionIndex].Correct.ToString(), PointsPerWrong)));
    }
    AdvanceQuestion();
    return bLastWasCorrect;
}

void UCalculusLesson::ApplyWrong(const FText& Message)
{
    bLastWasCorrect = false;
    Points -= PointsPerWrong;
    Feedback = Message;
}

void UCalculusLesson::AdvanceQuestion()
{
    if (++QuestionIndex < LessonQuestions.Num()) PrepareQuestion();
    else if (bHasBoard) StartBoard();
    else FinishLesson();
    Changed();
}

void UCalculusLesson::StartBoard()
{
    Phase = ECalcPhase::Board;
    const TArray<FText> Steps = Board.GetSteps();
    BoardShuffle.Reset();
    for (int32 i = 0; i < Steps.Num(); ++i) BoardShuffle.Add(i);
    // Garante que os passos não apareçam já na ordem certa.
    for (int32 Try = 0; Try < 10; ++Try)
    {
        Shuffle(BoardShuffle, Random);
        bool bIdentity = true;
        for (int32 i = 0; i < BoardShuffle.Num(); ++i) bIdentity &= BoardShuffle[i] == i;
        if (!bIdentity) break;
    }
    BoardSteps.Reset();
    for (int32 Original : BoardShuffle) BoardSteps.Add(Steps[Original]);
    TimeLeft = GetQuestionTime() * BoardTimeMultiplier;
}

bool UCalculusLesson::SubmitBoardOrder(const TArray<int32>& DisplayedOrder)
{
    if (Phase != ECalcPhase::Board || DisplayedOrder.Num() != BoardShuffle.Num()) return false;
    bool bCorrect = true;
    for (int32 Position = 0; Position < DisplayedOrder.Num(); ++Position)
        bCorrect &= BoardShuffle.IsValidIndex(DisplayedOrder[Position]) && BoardShuffle[DisplayedOrder[Position]] == Position;
    if (bCorrect)
    {
        bLastWasCorrect = bBoardSolved = true;
        Points += BoardPoints;
        Feedback = FText::FromString(FString::Printf(TEXT("Quadro resolvido! +%d"), BoardPoints));
        FinishLesson();
    }
    else
    {
        ApplyWrong(FText::FromString(FString::Printf(TEXT("Ordem errada, tente de novo (-%d)"), PointsPerWrong)));
    }
    Changed();
    return bCorrect;
}

void UCalculusLesson::Tick(float DeltaSeconds)
{
    if ((Phase != ECalcPhase::Question && Phase != ECalcPhase::Board) || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    TimeLeft = FMath::Max(0.f, TimeLeft - DeltaSeconds);
    if (TimeLeft > 0.f) return;
    if (Phase == ECalcPhase::Question)
    {
        ApplyWrong(FText::FromString(FString::Printf(TEXT("Tempo esgotado! Resposta: %s  (-%d)"), *LessonQuestions[QuestionIndex].Correct.ToString(), PointsPerWrong)));
        AdvanceQuestion();
    }
    else
    {
        // Quadro sem solução no tempo: sem pontos extras, encerra a aula.
        bLastWasCorrect = false;
        Feedback = FText::FromString(TEXT("Tempo do quadro esgotado."));
        FinishLesson();
        Changed();
    }
}

void UCalculusLesson::FinishLesson()
{
    Phase = ECalcPhase::Result;
    TimeLeft = 0.f;
    const int32 Max = GetMaxPoints();
    // Nota 0-100 proporcional aos pontos; nunca negativa.
    FinalGrade = Max > 0 ? FMath::Clamp(100.f * Points / Max, 0.f, 100.f) : 0.f;
}

bool UCalculusLesson::Retry()
{
    if (Phase != ECalcPhase::Result) return false;
    Phase = ECalcPhase::ChooseDifficulty;
    Feedback = FText::GetEmpty(); // não leva a mensagem da aula anterior para a tela de escolha
    Changed();
    return true;
}

float UCalculusLesson::GetQuestionTime() const
{
    switch (Difficulty)
    {
    case ECalcDifficulty::Medium: return MediumQuestionTime;
    case ECalcDifficulty::Hard: return HardQuestionTime;
    default: return EasyQuestionTime;
    }
}

int32 UCalculusLesson::GetMaxPoints() const
{
    return LessonQuestions.Num() * PointsPerCorrect + (bHasBoard ? BoardPoints : 0);
}

FName UCalculusLesson::LessonIdFor(ECalcDifficulty InDifficulty)
{
    switch (InDifficulty)
    {
    case ECalcDifficulty::Medium: return TEXT("Calculus01_Medium");
    case ECalcDifficulty::Hard: return TEXT("Calculus01_Hard");
    default: return TEXT("Calculus01_Easy");
    }
}

FText UCalculusLesson::DifficultyName(ECalcDifficulty InDifficulty)
{
    switch (InDifficulty)
    {
    case ECalcDifficulty::Medium: return FText::FromString(TEXT("Médio"));
    case ECalcDifficulty::Hard: return FText::FromString(TEXT("Difícil"));
    default: return FText::FromString(TEXT("Fácil"));
    }
}
