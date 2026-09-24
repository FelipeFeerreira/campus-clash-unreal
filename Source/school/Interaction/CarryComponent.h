#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CarryComponent.generated.h"

class ACarryableItem;

/** O que o E faria agora (para executar e para o texto do HUD). */
UENUM(BlueprintType)
enum class ECarryAction : uint8 { None, Station, PickUp, Drop };

/**
 * Carrega 1 item por vez (estilo Overcooked). Pode ficar no PlayerController ou no próprio pawn:
 * o "carregador" é o pawn controlado ou, se não houver controller, o dono do componente.
 * Prioridade do E: estação/bancada perto -> item solto perto (mãos vazias) -> soltar no chão.
 */
UCLASS(ClassGroup=(Campus), meta=(BlueprintSpawnableComponent))
class SCHOOL_API UCarryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ACarryableItem> Held;

    /** Posição do item carregado, relativa ao personagem (frente e altura do peito). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry") FVector HoldOffset = FVector(55.f, 0.f, 15.f);
    /** Distância máxima para pegar item solto ou usar estação (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry") float InteractRange = 180.f;
    /** A que distância à frente o item cai quando solto no chão (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Carry") float DropDistance = 80.f;

    UFUNCTION(BlueprintPure) bool IsHolding() const { return Held != nullptr; }
    UFUNCTION(BlueprintPure) AActor* GetCarrier() const;

    /** Segura o item (falha se já estiver segurando outro ou se ele tiver dono). */
    UFUNCTION(BlueprintCallable) bool PickUp(ACarryableItem* Item);
    /** Tira o item das mãos sem posicioná-lo (quem chama decide onde ele vai). */
    UFUNCTION(BlueprintCallable) ACarryableItem* TakeHeld();
    /** Solta o item no chão à frente do personagem. */
    UFUNCTION(BlueprintCallable) bool DropOnFloor();

    /** Decide o que o E faria agora; OutTarget recebe a estação ou o item alvo. */
    ECarryAction FindAction(AActor*& OutTarget) const;
    /** Executa a ação do E. Retorna true se algo aconteceu. */
    UFUNCTION(BlueprintCallable) bool Interact();
    /** Texto para o HUD ("[E] Pegar resistor 4 Ω", "[E] Soltar"...); vazio se nada a fazer. */
    UFUNCTION(BlueprintPure) FText GetPrompt() const;
    /** Distância até o alvo da ação atual (para comparar com outras interações). */
    float GetActionDistance() const;
};
