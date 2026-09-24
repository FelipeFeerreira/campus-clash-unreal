#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StudentProgress.generated.h"

UCLASS()
class SCHOOL_API UCampusCircuitSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version = 1;
    UPROPERTY(SaveGame) TMap<FName, float> BestGrades;
    UPROPERTY(SaveGame) TSet<FName> RewardedLessons;
    UPROPERTY(SaveGame) int32 Reputation = 0;
    UPROPERTY(SaveGame) bool bElectroShockUnlocked = false;
    bool RecordGrade(FName Lesson, float Grade);
};

UCLASS()
class SCHOOL_API UStudentProgress : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UCampusCircuitSave> Progress;
    UPROPERTY(BlueprintReadOnly) bool bLastSaveSucceeded = true;
    UPROPERTY(BlueprintReadOnly) bool bLoadFailed = false;
    UFUNCTION(BlueprintCallable) bool AddGrade(FName Lesson, float Grade);
    UFUNCTION(BlueprintCallable) bool SaveProgress();
    UFUNCTION(BlueprintPure) float GetGrade(FName Lesson) const;
};
