#include "CalculusClassroom.h"
#include "CalculusLesson.h"
#include "CalculusWidgets.h"
#include "Circuit/StudentProgress.h"
#include "schoolPlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACalculusClassroom::ACalculusClassroom()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Professor = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Professor"));
    Professor->SetupAttachment(RootComponent);
    // O manequim olha para +Y no espaço do modelo; -90° faz ele olhar para a frente do ator (+X).
    Professor->SetRelativeRotation(FRotator(0, -90, 0));
    Professor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (Manny.Succeeded()) Professor->SetSkeletalMesh(Manny.Object);
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
    if (Idle.Succeeded()) IdleAnimation = Idle.Object;

    Lesson = CreateDefaultSubobject<UCalculusLesson>(TEXT("Lesson"));
    ProfessorName = FText::FromString(TEXT("Professor de Cálculo"));
    IntroLines = {
        FText::FromString(TEXT("Bom dia! Hoje é aula de Cálculo I: limites, derivadas e integrais.")),
        FText::FromString(TEXT("Primeiro, escolha a dificuldade. Depois vêm 5 perguntas, cada uma com tempo.")),
        FText::FromString(TEXT("No fim, você resolve um problema no quadro, colocando os passos na ordem certa.")),
    };
}

void ACalculusClassroom::BeginPlay()
{
    Super::BeginPlay();
    FacingYaw = Professor->GetComponentRotation().Yaw;
    if (IdleAnimation) Professor->PlayAnimation(IdleAnimation, true);

    // Carrega o banco de perguntas das DataTables.
    TArray<FCalcQuestionRow> Questions;
    TArray<FCalcBoardRow> Boards;
    if (QuestionTable) QuestionTable->ForeachRow<FCalcQuestionRow>(TEXT("Calculus"), [&](const FName&, const FCalcQuestionRow& Row) { Questions.Add(Row); });
    if (BoardTable) BoardTable->ForeachRow<FCalcBoardRow>(TEXT("Calculus"), [&](const FName&, const FCalcBoardRow& Row) { Boards.Add(Row); });
    Lesson->SetContent(Questions, Boards);
    Lesson->OnChanged.AddDynamic(this, &ACalculusClassroom::HandleLessonChanged);

    GetWorldTimerManager().SetTimer(StartTimer, this, &ACalculusClassroom::BeginLesson, FMath::Max(0.01f, StartDelay));
}

void ACalculusClassroom::BeginLesson()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC || Widget) return;
    Widget = CreateWidget<UCalculusWidget>(PC);
    Widget->Room = this;
    Widget->AddToViewport(20);
    // Durante a aula o jogador não anda: toda a entrada vai para a interface.
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(Widget->TakeWidget());
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = true;
    if (auto* SchoolPC = Cast<AschoolPlayerController>(PC)) SchoolPC->SetCircuitHUDVisible(false);
    IntroIndex = 0;
    Lesson->StartLesson();
    if (IntroLines.Num() == 0) Lesson->FinishIntro();
}

void ACalculusClassroom::AdvanceIntro()
{
    if (Lesson->Phase != ECalcPhase::Intro) return;
    if (++IntroIndex >= IntroLines.Num()) Lesson->FinishIntro();
}

FText ACalculusClassroom::GetIntroLine() const
{
    return IntroLines.IsValidIndex(IntroIndex) ? IntroLines[IntroIndex] : FText::GetEmpty();
}

void ACalculusClassroom::HandleLessonChanged()
{
    // Grava a nota uma vez por aula concluída; "Repetir" libera a próxima gravação.
    if (Lesson->Phase == ECalcPhase::Result && !bGradeRecorded)
    {
        bGradeRecorded = true;
        auto* Progress = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStudentProgress>() : nullptr;
        bSaveFailed = !Progress || !Progress->AddGrade(Lesson->GetLessonId(), Lesson->FinalGrade);
        UE_LOG(LogTemp, Log, TEXT("Calculus %s: pontos=%d nota=%.0f acertos=%d/%d quadro=%d"),
            *Lesson->GetLessonId().ToString(), Lesson->Points, Lesson->FinalGrade, Lesson->CorrectAnswers, Lesson->LessonQuestions.Num(), Lesson->bBoardSolved);
    }
    else if (Lesson->Phase != ECalcPhase::Result) bGradeRecorded = false;
}

void ACalculusClassroom::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Lesson->Tick(DeltaSeconds);

    // Enquanto fala a introdução, o professor se vira para o jogador; depois volta para a turma.
    float TargetYaw = GetActorRotation().Yaw - 90.f;
    if (Lesson->Phase == ECalcPhase::Intro)
        if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
            TargetYaw = (Pawn->GetActorLocation() - Professor->GetComponentLocation()).Rotation().Yaw - 90.f;
    FacingYaw = FMath::FixedTurn(FacingYaw, TargetYaw, ProfessorTurnSpeed * 60.f * DeltaSeconds);
    Professor->SetWorldRotation(FRotator(0, FacingYaw, 0));
}

void ACalculusClassroom::LeaveToCampus()
{
    // Devolve o controle ao jogo antes de trocar de nível; senão o modo "só UI" continua
    // no viewport e o personagem não anda no campus.
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
    if (Widget) Widget->RemoveFromParent();
    UGameplayStatics::OpenLevel(this, CampusLevel);
}

float ACalculusClassroom::GetBestGrade(ECalcDifficulty InDifficulty) const
{
    const auto* Progress = GetGameInstance() ? GetGameInstance()->GetSubsystem<UStudentProgress>() : nullptr;
    return Progress ? Progress->GetGrade(UCalculusLesson::LessonIdFor(InDifficulty)) : 0.f;
}
