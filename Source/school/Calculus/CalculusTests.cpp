#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CalculusLesson.h"
#include "Circuit/StudentProgress.h"

namespace CalcTests
{
    FCalcQuestionRow Question(ECalcDifficulty Difficulty, const TCHAR* Correct)
    {
        FCalcQuestionRow Q;
        Q.Difficulty = Difficulty;
        Q.Question = FText::FromString(TEXT("Pergunta"));
        Q.Correct = FText::FromString(Correct);
        Q.Wrong1 = FText::FromString(TEXT("errada A"));
        Q.Wrong2 = FText::FromString(TEXT("errada B"));
        Q.Wrong3 = FText::FromString(TEXT("errada C"));
        return Q;
    }

    FCalcBoardRow Board(ECalcDifficulty Difficulty)
    {
        FCalcBoardRow B;
        B.Difficulty = Difficulty;
        B.Step1 = FText::FromString(TEXT("passo 1"));
        B.Step2 = FText::FromString(TEXT("passo 2"));
        B.Step3 = FText::FromString(TEXT("passo 3"));
        return B;
    }

    /** Aula com 6 perguntas fáceis, 1 média e um quadro fácil; já na escolha de dificuldade. */
    UCalculusLesson* Lesson()
    {
        auto* L = NewObject<UCalculusLesson>();
        TArray<FCalcQuestionRow> Qs;
        for (int32 i = 0; i < 6; ++i) Qs.Add(Question(ECalcDifficulty::Easy, *FString::Printf(TEXT("certa %d"), i)));
        Qs.Add(Question(ECalcDifficulty::Medium, TEXT("certa media")));
        L->SetContent(Qs, { Board(ECalcDifficulty::Easy) });
        L->SetSeed(1234);
        L->StartLesson();
        L->FinishIntro();
        return L;
    }

    /** Ordem de exibição que resolve o quadro. */
    TArray<int32> Solution(const UCalculusLesson* L)
    {
        TArray<int32> Order;
        for (int32 Correct = 0; Correct < L->BoardShuffle.Num(); ++Correct)
            Order.Add(L->BoardShuffle.IndexOfByKey(Correct));
        return Order;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalcFlowTest, "Campus.Calculus.FlowAndScoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalcFlowTest::RunTest(const FString&)
{
    auto* L = CalcTests::Lesson();
    TestFalse(TEXT("Não responde antes de escolher"), L->AnswerQuestion(0));
    TestTrue(TEXT("Escolhe fácil"), L->ChooseDifficulty(ECalcDifficulty::Easy));
    TestEqual(TEXT("5 perguntas sorteadas"), L->LessonQuestions.Num(), 5);
    for (const auto& Q : L->LessonQuestions) TestTrue(TEXT("Só da dificuldade escolhida"), Q.Difficulty == ECalcDifficulty::Easy);
    TestEqual(TEXT("Timer fácil 30 s"), L->TimeLeft, 30.f);
    TestEqual(TEXT("4 opções"), L->Options.Num(), 4);
    TestTrue(TEXT("Opção correta é a resposta certa"), L->Options[L->CorrectOption].EqualTo(L->LessonQuestions[0].Correct));

    TestTrue(TEXT("Acerto"), L->AnswerQuestion(L->CorrectOption));
    TestEqual(TEXT("+10"), L->Points, 10);
    TestFalse(TEXT("Erro"), L->AnswerQuestion((L->CorrectOption + 1) % 4));
    TestEqual(TEXT("-5"), L->Points, 5);
    L->Tick(31.f);
    TestEqual(TEXT("Tempo esgotado conta como erro"), L->Points, 0);
    TestEqual(TEXT("Tempo esgotado avança"), L->QuestionIndex, 3);
    L->AnswerQuestion(L->CorrectOption);
    L->AnswerQuestion(L->CorrectOption);
    TestTrue(TEXT("Depois das perguntas vem o quadro"), L->Phase == ECalcPhase::Board);
    TestEqual(TEXT("Timer do quadro = 2x"), L->TimeLeft, 60.f);

    auto Wrong = CalcTests::Solution(L); Wrong.Swap(0, 1);
    TestFalse(TEXT("Ordem errada"), L->SubmitBoardOrder(Wrong));
    TestEqual(TEXT("Ordem errada -5"), L->Points, 15);
    TestTrue(TEXT("Continua no quadro"), L->Phase == ECalcPhase::Board);
    TestTrue(TEXT("Ordem certa"), L->SubmitBoardOrder(CalcTests::Solution(L)));
    TestTrue(TEXT("Resultado"), L->Phase == ECalcPhase::Result);
    TestEqual(TEXT("Pontos finais"), L->Points, 35);
    TestEqual(TEXT("Máximo 70"), L->GetMaxPoints(), 70);
    TestEqual(TEXT("Nota 50"), L->FinalGrade, 50.f);
    TestEqual(TEXT("Id por dificuldade"), L->GetLessonId(), FName(TEXT("Calculus01_Easy")));
    TestTrue(TEXT("Repetir volta à escolha"), L->Retry());
    TestTrue(TEXT("Fase de escolha"), L->Phase == ECalcPhase::ChooseDifficulty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalcEdgeTest, "Campus.Calculus.EdgeCases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCalcEdgeTest::RunTest(const FString&)
{
    // Poucas perguntas e sem quadro: usa as que houver; nota nunca negativa.
    auto* L = CalcTests::Lesson();
    L->ChooseDifficulty(ECalcDifficulty::Medium);
    TestEqual(TEXT("Só 1 pergunta média disponível"), L->LessonQuestions.Num(), 1);
    TestEqual(TEXT("Timer médio 45 s"), L->TimeLeft, 45.f);
    L->AnswerQuestion((L->CorrectOption + 1) % 4);
    TestTrue(TEXT("Sem quadro vai direto ao resultado"), L->Phase == ECalcPhase::Result);
    TestEqual(TEXT("Pontos negativos"), L->Points, -5);
    TestEqual(TEXT("Nota não fica negativa"), L->FinalGrade, 0.f);
    TestFalse(TEXT("Tick no resultado não muda nada"), (L->Tick(100.f), L->Phase != ECalcPhase::Result));
    TestFalse(TEXT("Índice inválido rejeitado"), L->AnswerQuestion(7));

    // Quadro com tempo esgotado: encerra sem os pontos do quadro.
    auto* B = CalcTests::Lesson();
    B->ChooseDifficulty(ECalcDifficulty::Easy);
    for (int32 i = 0; i < 5; ++i) B->AnswerQuestion(B->CorrectOption);
    bool bIdentity = true;
    for (int32 i = 0; i < B->BoardShuffle.Num(); ++i) bIdentity &= B->BoardShuffle[i] == i;
    TestFalse(TEXT("Quadro não começa já na ordem certa"), bIdentity);
    B->Tick(1000.f);
    TestTrue(TEXT("Timeout do quadro encerra"), B->Phase == ECalcPhase::Result);
    TestFalse(TEXT("Quadro não resolvido"), B->bBoardSolved);
    TestEqual(TEXT("Nota 50/70"), B->FinalGrade, 100.f * 50 / 70);

    // Cálculo não desbloqueia o Electro-Shock (recompensa exclusiva de circuitos).
    auto* Save = NewObject<UCampusCircuitSave>();
    Save->RecordGrade(TEXT("Calculus01_Easy"), 80.f);
    TestEqual(TEXT("Cálculo dá reputação"), Save->Reputation, 2);
    TestFalse(TEXT("Cálculo não libera Electro-Shock"), Save->bElectroShockUnlocked);
    return true;
}
#endif
