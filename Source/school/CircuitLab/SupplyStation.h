#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/CampusInteractable.h"
#include "SupplyStation.generated.h"

class ACarryableItem;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Bancada de estoque (estilo Overcooked): estoque infinito de um tipo de item.
 * Mãos vazias + E = pega um item com valor sorteado da lista; segurando o mesmo tipo + E = devolve.
 */
UCLASS(Blueprintable)
class SCHOOL_API ASupplyStation : public AActor, public ICampusInteractable
{
    GENERATED_BODY()
public:
    ASupplyStation();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Counter;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Sample;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTextRenderComponent> Sign;

    /** Tipo entregue (Battery, Source, Resistor, Lamp, Wire). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") FName ItemType = TEXT("Resistor");
    /** Nome base do item, ex.: "Resistor". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") FText ItemName;
    /** Placa da bancada, ex.: "RESISTORES". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") FText SignText;
    /** Unidade mostrada depois do valor ("V", "ohm"); vazio = item sem valor (fio). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") FString Unit = TEXT("ohm");
    /** Valores possíveis; cada item pego sorteia um. Vazio = valor 0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") TArray<float> Values = { 1, 2, 3, 4, 5, 6, 8, 10 };
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") FLinearColor Color = FLinearColor(.85f, .45f, .12f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station") TSubclassOf<ACarryableItem> ItemClass;

    /** Cria um item novo (valor sorteado), sem entregá-lo a ninguém. */
    UFUNCTION(BlueprintCallable) ACarryableItem* SpawnItem();
    /** Configura tipo/nome/valores de uma vez (usado pelo script de montagem e testes). */
    void Configure(FName InType, const FString& InName, const FString& InSign, const FString& InUnit, const TArray<float>& InValues, const FLinearColor& InColor);
    /** "9 V", "1,5 V", "4 ohm"; sem unidade, só o nome. */
    static FString FormatValue(float InValue, const FString& InUnit);

    // ICampusInteractable
    virtual bool CanInteract(const APawn* User, const ACarryableItem* Held) const override;
    virtual bool Interact(APawn* User, UCarryComponent* Carry) override;
    virtual FText GetInteractPrompt(const ACarryableItem* Held) const override;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    void RefreshAppearance();
};
