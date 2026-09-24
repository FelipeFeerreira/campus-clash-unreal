// Copyright Epic Games, Inc. All Rights Reserved.


#include "schoolPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "school.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Circuit/CircuitActors.h"
#include "Circuit/CircuitManager.h"
#include "Circuit/CircuitWidgets.h"
#include "Circuit/StudentProgress.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "schoolCharacter.h"
#include "Interaction/CarryComponent.h"
#include "GameFramework/SpringArmComponent.h"

AschoolPlayerController::AschoolPlayerController()
{
    Carry = CreateDefaultSubobject<UCarryComponent>(TEXT("Carry"));
}

void AschoolPlayerController::BeginPlay()
{
	Super::BeginPlay();
    if (IsLocalController())
    {
        CircuitHUD = CreateWidget<UCircuitHUDWidget>(this);
        CircuitHUD->AddToViewport(2);
    }

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(Logschool, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AschoolPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AschoolPlayerController::CircuitInteract);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &AschoolPlayerController::CircuitGrabOrPlace);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AschoolPlayerController::CloseCircuitPanel);

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AschoolPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

AActor* AschoolPlayerController::FindCircuitInteraction() const
{
    const APawn* PlayerPawn = GetPawn();
    if (!PlayerPawn) return nullptr;
    AActor* Best = nullptr;
    float BestDistance = FMath::Square(290.f);
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        const auto* Ext = Cast<AExtinguisherActor>(Actor);
        if (!Cast<ADoorActor>(Actor) && !Cast<ACircuitWorkbench>(Actor) && !(Ext && !Ext->Holder)) continue;
        const float Distance = FVector::DistSquared(PlayerPawn->GetActorLocation(),Actor->GetActorLocation());
        if (Distance < BestDistance) { BestDistance=Distance; Best=Actor; }
    }
    return Best;
}

void AschoolPlayerController::CircuitInteract()
{
    if (CircuitPanel) { CloseCircuitPanel(); return; }
    AActor* Target = FindCircuitInteraction();
    // Sistema de carregar itens: usa a interação mais próxima (soltar no chão só se não houver outra).
    const float OldDistance = Target && GetPawn() ? FVector::Dist2D(Target->GetActorLocation(), GetPawn()->GetActorLocation()) : TNumericLimits<float>::Max();
    if (Carry && Carry->GetActionDistance() < OldDistance) { Carry->Interact(); return; }
    if (auto* Door = Cast<ADoorActor>(Target)) Door->OpenDoor(GetPawn());
    else if (auto* Ext = Cast<AExtinguisherActor>(Target))
    {
        if (HeldCircuitPart) { HeldCircuitPart->Release(); HeldCircuitPart=nullptr; }
        if (Carry && Carry->IsHolding()) Carry->DropOnFloor(); // o extintor ocupa as mãos
        if (Ext->Grab(GetPawn())) HeldExtinguisher=Ext;
    }
    else if (auto* Bench=Cast<ACircuitWorkbench>(Target))
    {
        if (HeldCircuitPart) { HeldCircuitPart->Release(); HeldCircuitPart=nullptr; }
        CircuitPanel=CreateWidget<UCircuitWorkbenchWidget>(this);
        CircuitPanel->Bench=Bench;
        CircuitPanel->AddToViewport(10);
        SetIgnoreMoveInput(true); SetIgnoreLookInput(true);
        bShowMouseCursor=true;
        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(CircuitPanel->TakeWidget()); Mode.SetHideCursorDuringCapture(false);
        SetInputMode(Mode);
    }
}

void AschoolPlayerController::CloseCircuitPanel()
{
    if (!CircuitPanel) return;
    CircuitPanel->RemoveFromParent(); CircuitPanel=nullptr;
    SetIgnoreMoveInput(false); SetIgnoreLookInput(false);
    bShowMouseCursor=false; SetInputMode(FInputModeGameOnly());
}

void AschoolPlayerController::CircuitGrabOrPlace()
{
    if (CircuitPanel || !GetPawn()) return;
    if (HeldCircuitPart)
    {
        auto* Bench=HeldCircuitPart->Bench.Get();
        const FName Slot = HeldCircuitPart->Data.Type == ECircuitPartType::PowerSource ? TEXT("Source") : HeldCircuitPart->Data.Type == ECircuitPartType::Bulb ? TEXT("Bulb") : TEXT("Resistor");
        if (!Bench || FVector::DistSquared(GetPawn()->GetActorLocation(),Bench->GetActorLocation()) > FMath::Square(280.f)
            || !Bench->AddComponentToSlot(HeldCircuitPart,Slot)) HeldCircuitPart->Release();
        HeldCircuitPart=nullptr;
        return;
    }
    FVector Eye; FRotator Aim; GetPlayerViewPoint(Eye,Aim);
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(CircuitGrab),true,GetPawn());
    if (HeldExtinguisher) Params.AddIgnoredActor(HeldExtinguisher);
    if (GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Aim.Vector()*800,ECC_Visibility,Params))
        if (auto* Part=Cast<AComponentBox>(Hit.GetActor()))
            if (FVector::DistSquared(GetPawn()->GetActorLocation(),Part->GetActorLocation()) < FMath::Square(280.f) && Part->Grab(GetPawn())) HeldCircuitPart=Part;
}

void AschoolPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsValid(HeldExtinguisher)) HeldExtinguisher->Spray(!CircuitPanel && IsInputKeyDown(EKeys::F),DeltaSeconds);
    UpdateExtinguisherCamera(DeltaSeconds);
}

void AschoolPlayerController::UpdateExtinguisherCamera(float DeltaSeconds)
{
    const auto* PlayerCharacter = Cast<AschoolCharacter>(GetPawn());
    USpringArmComponent* Boom = PlayerCharacter ? PlayerCharacter->GetCameraBoom() : nullptr;
    if (!Boom) return;
    // Guarda os valores do Blueprint do personagem na primeira vez que vemos este pawn.
    if (CameraDefaultsPawn.Get() != PlayerCharacter)
    {
        CameraDefaultsPawn = const_cast<AschoolCharacter*>(PlayerCharacter);
        DefaultCameraSocketOffset = Boom->SocketOffset;
        DefaultCameraArmLength = Boom->TargetArmLength;
    }
    const bool bShoulder = IsValid(HeldExtinguisher);
    const FVector TargetOffset = bShoulder ? ExtinguisherShoulderOffset : DefaultCameraSocketOffset;
    const float TargetLength = bShoulder ? ExtinguisherArmLength : DefaultCameraArmLength;
    Boom->SocketOffset = FMath::VInterpTo(Boom->SocketOffset, TargetOffset, DeltaSeconds, ExtinguisherCameraBlendSpeed);
    Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetLength, DeltaSeconds, ExtinguisherCameraBlendSpeed);
}

void AschoolPlayerController::SetCircuitHUDVisible(bool bVisible)
{
    if (CircuitHUD) CircuitHUD->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

bool AschoolPlayerController::IsExtinguisherHittingFire() const { return IsValid(HeldExtinguisher) && HeldExtinguisher->bHitFire; }

FString AschoolPlayerController::GetCircuitPrompt() const
{
    FString Message=TEXT("TECHRIDGE / CIRCUITOS I\n");
    if (auto* Progress=GetGameInstance()->GetSubsystem<UStudentProgress>())
    {
        Message+=FString::Printf(TEXT("Melhor nota: %.0f/100 | Reputacao: %d\n"),Progress->GetGrade(TEXT("CircuitSeries01")),Progress->Progress ? Progress->Progress->Reputation : 0);
        if (!Progress->bLastSaveSucceeded) Message+=TEXT("Aviso: progresso ainda nao salvo.\n");
    }
    AActor* Target=FindCircuitInteraction();
    // Mesmo critério do E: a interação de carregar itens mais próxima tem prioridade.
    const float OldDistance = Target && GetPawn() ? FVector::Dist2D(Target->GetActorLocation(), GetPawn()->GetActorLocation()) : TNumericLimits<float>::Max();
    const FText CarryPrompt = Carry ? Carry->GetPrompt() : FText::GetEmpty();
    if (!CarryPrompt.IsEmpty() && Carry->GetActionDistance() < OldDistance) Message+=CarryPrompt.ToString() + TEXT("\n");
    else if (const auto* Door=Cast<ADoorActor>(Target)) Message+=Door->bLocked ? TEXT("Porta bloqueada: apague todos os focos.\n") : Door->DoorTitle.ToString() + TEXT("\n"); // o título já traz "[E]"
    else if (Cast<AExtinguisherActor>(Target)) Message+=TEXT("[E] Pegar extintor\n");
    else if (Cast<ACircuitWorkbench>(Target)) Message+=TEXT("[E] Montar circuito | [G] Pegar/encaixar peca sob a mira\n");
    else Message+=TEXT("Procure a porta CIRCUITOS I no campus.\n");
    if (HeldCircuitPart) Message+=TEXT("[G] Encaixar peca na bancada ou devolver\n");
    if (HeldExtinguisher) Message+=TEXT("Segure [F] e mire + nos focos. Alcance: 5 m.\n");
    for (TActorIterator<ACircuitWorkbench> It(GetWorld()); It; ++It)
        if (It->Circuit->Phase==ECircuitPhase::Burning)
        {
            // Singular/plural: "1 foco ativo", "2 focos ativos".
            const int32 Count = It->Fires.Num();
            Message+=FString::Printf(TEXT("ALERTA: %d %s. Extintor ao lado da bancada."),Count,Count==1 ? TEXT("foco ativo") : TEXT("focos ativos"));
        }
    return Message;
}
