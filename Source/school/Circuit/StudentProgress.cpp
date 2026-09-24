#include "StudentProgress.h"
#include "Kismet/GameplayStatics.h"

namespace { const FString CircuitSlot = TEXT("CampusClash_Profile01_Circuits_v1"); }

bool UCampusCircuitSave::RecordGrade(FName Lesson, float Grade)
{
    if (Lesson.IsNone() || !FMath::IsFinite(Grade)) return false;
    Grade = FMath::Clamp(Grade, 0.f, 100.f);
    BestGrades.FindOrAdd(Lesson) = FMath::Max(BestGrades.FindRef(Lesson), Grade);
    if (Grade >= 60.f && !RewardedLessons.Contains(Lesson))
    {
        RewardedLessons.Add(Lesson);
        Reputation += 2;
        // Electro-Shock é recompensa só das aulas de circuitos (LessonID começando com "Circuit").
        if (Lesson.ToString().StartsWith(TEXT("Circuit"))) bElectroShockUnlocked = true;
    }
    return true;
}

void UStudentProgress::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UGameplayStatics::DoesSaveGameExist(CircuitSlot, 0))
    {
        Progress = Cast<UCampusCircuitSave>(UGameplayStatics::LoadGameFromSlot(CircuitSlot, 0));
        bLoadFailed = !Progress || Progress->Version != 1;
        if (bLoadFailed) Progress = nullptr;
    }
    if (!Progress) Progress = Cast<UCampusCircuitSave>(UGameplayStatics::CreateSaveGameObject(UCampusCircuitSave::StaticClass()));
    bLastSaveSucceeded = !bLoadFailed;
}

bool UStudentProgress::AddGrade(FName Lesson, float Grade)
{
    if (!Progress || !Progress->RecordGrade(Lesson, Grade)) return false;
    return SaveProgress();
}

bool UStudentProgress::SaveProgress()
{
    // Never overwrite an unreadable or newer-version player save.
    bLastSaveSucceeded = !bLoadFailed && Progress && UGameplayStatics::SaveGameToSlot(Progress, CircuitSlot, 0);
    return bLastSaveSucceeded;
}

float UStudentProgress::GetGrade(FName Lesson) const { return Progress ? Progress->BestGrades.FindRef(Lesson) : 0.f; }
