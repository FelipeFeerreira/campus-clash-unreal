#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CampusInteractable.generated.h"

class APawn;
class ACarryableItem;
class UCarryComponent;

UINTERFACE(MinimalAPI)
class UCampusInteractable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Qualquer coisa que responde ao E do sistema de carregar itens
 * (estação de estoque, bancada de montagem, lixeira...). Reaproveitável em outras aulas.
 */
class SCHOOL_API ICampusInteractable
{
    GENERATED_BODY()
public:
    /** Pode interagir agora, segurando (ou não) este item? */
    virtual bool CanInteract(const APawn* User, const ACarryableItem* Held) const = 0;
    /** Executa a interação (pegar do estoque, colocar na bancada...). Retorna true se algo aconteceu. */
    virtual bool Interact(APawn* User, UCarryComponent* Carry) = 0;
    /** Texto do HUD, ex.: "[E] Pegar resistor". */
    virtual FText GetInteractPrompt(const ACarryableItem* Held) const = 0;
};
