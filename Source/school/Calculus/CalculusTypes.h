#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CalculusTypes.generated.h"

/** Dificuldade escolhida pelo jogador no início da aula. */
UENUM(BlueprintType)
enum class ECalcDifficulty : uint8 { Easy, Medium, Hard };

/** Assunto da pergunta (usado para filtrar/balancear no futuro). */
UENUM(BlueprintType)
enum class ECalcTopic : uint8 { Limit, Derivative, Integral };

/** Etapas da aula, na ordem: Intro -> escolha -> perguntas -> quadro -> resultado. */
UENUM(BlueprintType)
enum class ECalcPhase : uint8 { Idle, Intro, ChooseDifficulty, Question, Board, Result };

/** Uma pergunta de múltipla escolha. Linha de DT_CalculusQuestions (CalculusQuestions.csv). */
USTRUCT(BlueprintType)
struct SCHOOL_API FCalcQuestionRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ECalcDifficulty Difficulty = ECalcDifficulty::Easy;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ECalcTopic Topic = ECalcTopic::Derivative;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Question;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Correct;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Wrong1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Wrong2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Wrong3;
    /** Marca perguntas de exemplo, para trocar pelas definitivas. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bProvisional = false;
};

/** Desafio do quadro: passos da resolução na ordem certa (vazios são ignorados). Linha de DT_CalculusBoard. */
USTRUCT(BlueprintType)
struct SCHOOL_API FCalcBoardRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ECalcDifficulty Difficulty = ECalcDifficulty::Easy;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Problem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Step1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Step2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Step3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Step4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Step5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Step6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bProvisional = false;

    /** Passos preenchidos, na ordem correta. */
    TArray<FText> GetSteps() const
    {
        TArray<FText> Steps;
        for (const FText* Step : { &Step1, &Step2, &Step3, &Step4, &Step5, &Step6 })
            if (!Step->IsEmptyOrWhitespace()) Steps.Add(*Step);
        return Steps;
    }
};
