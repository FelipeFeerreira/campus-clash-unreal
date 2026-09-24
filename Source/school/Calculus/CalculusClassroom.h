#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalculusTypes.h"
#include "CalculusClassroom.generated.h"

class UCalculusLesson;
class UCalculusWidget;
class UDataTable;
class UAnimationAsset;
class USkeletalMeshComponent;

/**
 * Aula de Cálculo na sala: professor, lógica (UCalculusLesson), timer, save e interface.
 * A aula começa sozinha quando o nível carrega; o jogador fica parado enquanto ela acontece.
 */
UCLASS(Blueprintable)
class SCHOOL_API ACalculusClassroom : public AActor
{
    GENERATED_BODY()
public:
    ACalculusClassroom();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USkeletalMeshComponent> Professor;
    /** Lógica e ajustes da aula (tempos, pontos) — editáveis no Details. */
    UPROPERTY(VisibleAnywhere, Instanced, BlueprintReadOnly, Category="Calculus") TObjectPtr<UCalculusLesson> Lesson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus") TObjectPtr<UDataTable> QuestionTable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus") TObjectPtr<UDataTable> BoardTable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Professor") FText ProfessorName;
    /** Falas de introdução, uma por caixa de texto. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Professor") TArray<FText> IntroLines;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Professor") TObjectPtr<UAnimationAsset> IdleAnimation;
    /** Velocidade com que o professor se vira para o jogador ao falar. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus|Professor") float ProfessorTurnSpeed = 4.f;
    /** Segundos depois de entrar na sala até o professor começar a falar. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus") float StartDelay = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Calculus") FName CampusLevel = TEXT("/Game/CampusClash/Maps/Lvl_Techridge");

    UPROPERTY(BlueprintReadOnly) int32 IntroIndex = 0;
    UPROPERTY(BlueprintReadOnly) bool bSaveFailed = false;

    UFUNCTION(BlueprintCallable) void BeginLesson();
    UFUNCTION(BlueprintCallable) void AdvanceIntro();
    UFUNCTION(BlueprintPure) FText GetIntroLine() const;
    UFUNCTION(BlueprintCallable) void LeaveToCampus();
    UFUNCTION(BlueprintPure) float GetBestGrade(ECalcDifficulty InDifficulty) const;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() TObjectPtr<UCalculusWidget> Widget;
    FTimerHandle StartTimer;
    bool bGradeRecorded = false;
    float FacingYaw = 0.f;
    UFUNCTION() void HandleLessonChanged();
};
