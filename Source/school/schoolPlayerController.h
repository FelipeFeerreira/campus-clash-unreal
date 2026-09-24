// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "schoolPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UCircuitWorkbenchWidget;
class UCircuitHUDWidget;
class AComponentBox;
class AExtinguisherActor;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AschoolPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    AschoolPlayerController();
    virtual void Tick(float DeltaSeconds) override;
    /** Sistema genérico de carregar itens (aulas estilo Overcooked). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Carry") TObjectPtr<class UCarryComponent> Carry;
    UFUNCTION(BlueprintCallable) void CloseCircuitPanel();
    FString GetCircuitPrompt() const;
    bool IsExtinguisherHittingFire() const;
    /** Mostra/esconde o HUD dos circuitos (ex.: durante a aula de Cálculo). */
    UFUNCTION(BlueprintCallable) void SetCircuitHUDVisible(bool bVisible);
private:
    UPROPERTY() TObjectPtr<UCircuitWorkbenchWidget> CircuitPanel;
    UPROPERTY() TObjectPtr<UCircuitHUDWidget> CircuitHUD;
    UPROPERTY() TObjectPtr<AComponentBox> HeldCircuitPart;
    UPROPERTY() TObjectPtr<AExtinguisherActor> HeldExtinguisher;
    void CircuitInteract();
    void CircuitGrabOrPlace();
    AActor* FindCircuitInteraction() const;
    /** Aproxima a câmera "por cima do ombro" enquanto o extintor está na mão e volta ao soltar. */
    void UpdateExtinguisherCamera(float DeltaSeconds);
    /** Valores originais do braço da câmera do pawn atual, restaurados sem o extintor. */
    TWeakObjectPtr<APawn> CameraDefaultsPawn;
    FVector DefaultCameraSocketOffset = FVector::ZeroVector;
    float DefaultCameraArmLength = 400.f;

protected:

	/** Deslocamento do braço da câmera com o extintor (Y positivo = ombro direito, onde o extintor fica). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Circuit|Camera")
	FVector ExtinguisherShoulderOffset = FVector(0.f, 70.f, 40.f);

	/** Comprimento do braço da câmera com o extintor, em cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Circuit|Camera", meta=(ClampMin="50"))
	float ExtinguisherArmLength = 250.f;

	/** Velocidade da transição entre câmera normal e de ombro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Circuit|Camera", meta=(ClampMin="0.1"))
	float ExtinguisherCameraBlendSpeed = 8.f;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
