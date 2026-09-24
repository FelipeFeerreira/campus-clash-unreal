#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CalculusTypes.h"
#include "CalculusLesson.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCalcLessonChanged);

/**
 * Lógica da aula de Cálculo, sem interface nem atores (testável isoladamente).
 * Fluxo: Intro -> ChooseDifficulty -> Question (N perguntas com timer) -> Board -> Result.
 * Tempos e pontos são editáveis no Blueprint da sala.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCHOOL_API UCalculusLesson : public UObject
{
    GENERATED_BODY()
public:
    // --- Ajustes (tuning) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning", meta=(ClampMin="1"))
    int32 QuestionsPerLesson = 5;
    /** Segundos por pergunta: fácil, médio, difícil. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    float EasyQuestionTime = 30.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    float MediumQuestionTime = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    float HardQuestionTime = 60.f;
    /** Tempo do quadro = tempo da pergunta × este fator. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    float BoardTimeMultiplier = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    int32 PointsPerCorrect = 10;
    /** Pontos perdidos por erro ou tempo esgotado (valor positivo). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    int32 PointsPerWrong = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Tuning")
    int32 BoardPoints = 20;

    // --- Estado (somente leitura para a UI) ---
    UPROPERTY(BlueprintReadOnly) ECalcPhase Phase = ECalcPhase::Idle;
    UPROPERTY(BlueprintReadOnly) ECalcDifficulty Difficulty = ECalcDifficulty::Easy;
    UPROPERTY(BlueprintReadOnly) TArray<FCalcQuestionRow> LessonQuestions;
    UPROPERTY(BlueprintReadOnly) int32 QuestionIndex = 0;
    /** Opções da pergunta atual, já embaralhadas. */
    UPROPERTY(BlueprintReadOnly) TArray<FText> Options;
    UPROPERTY(BlueprintReadOnly) int32 CorrectOption = 0;
    UPROPERTY(BlueprintReadOnly) float TimeLeft = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 Points = 0;
    UPROPERTY(BlueprintReadOnly) int32 CorrectAnswers = 0;
    UPROPERTY(BlueprintReadOnly) bool bHasBoard = false;
    UPROPERTY(BlueprintReadOnly) FCalcBoardRow Board;
    /** Passos do quadro embaralhados; BoardShuffle[i] = índice correto do passo exibido em i. */
    UPROPERTY(BlueprintReadOnly) TArray<FText> BoardSteps;
    UPROPERTY(BlueprintReadOnly) TArray<int32> BoardShuffle;
    UPROPERTY(BlueprintReadOnly) bool bBoardSolved = false;
    UPROPERTY(BlueprintReadOnly) float FinalGrade = 0.f;
    /** Mensagem curta para a UI ("Certo!", "Tempo esgotado"...). */
    UPROPERTY(BlueprintReadOnly) FText Feedback;
    UPROPERTY(BlueprintReadOnly) bool bLastWasCorrect = false;

    UPROPERTY(BlueprintAssignable) FCalcLessonChanged OnChanged;

    /** Carrega o banco de perguntas e desafios (normalmente das DataTables). */
    void SetContent(const TArray<FCalcQuestionRow>& InQuestions, const TArray<FCalcBoardRow>& InBoards);
    /** Semente fixa para testes; sem chamar, usa uma aleatória. */
    void SetSeed(int32 Seed) { Random.Initialize(Seed); }

    UFUNCTION(BlueprintCallable) bool StartLesson();
    UFUNCTION(BlueprintCallable) bool FinishIntro();
    UFUNCTION(BlueprintCallable) bool ChooseDifficulty(ECalcDifficulty InDifficulty);
    /** Responde a pergunta atual; retorna true se acertou. */
    UFUNCTION(BlueprintCallable) bool AnswerQuestion(int32 OptionIndex);
    /** Envia a ordem montada no quadro (índices das posições exibidas). Retorna true se correta. */
    UFUNCTION(BlueprintCallable) bool SubmitBoardOrder(const TArray<int32>& DisplayedOrder);
    /** Volta à escolha de dificuldade (botão Repetir). */
    UFUNCTION(BlueprintCallable) bool Retry();
    /** Avança os timers; chamado pelo ator da sala. */
    void Tick(float DeltaSeconds);

    UFUNCTION(BlueprintPure) float GetQuestionTime() const;
    UFUNCTION(BlueprintPure) int32 GetMaxPoints() const;
    UFUNCTION(BlueprintPure) FName GetLessonId() const { return LessonIdFor(Difficulty); }
    /** Id usado no save: melhor nota separada por dificuldade. */
    static FName LessonIdFor(ECalcDifficulty InDifficulty);
    static FText DifficultyName(ECalcDifficulty InDifficulty);

private:
    TArray<FCalcQuestionRow> AllQuestions;
    TArray<FCalcBoardRow> AllBoards;
    /** Semente nova a cada aula (FMath::Rand sem srand repetia a mesma ordem em toda partida). */
    FRandomStream Random{ static_cast<int32>(FPlatformTime::Cycles()) };

    void PrepareQuestion();
    void ApplyWrong(const FText& Message);
    void AdvanceQuestion();
    void StartBoard();
    void FinishLesson();
    void Changed() { OnChanged.Broadcast(); }
};
