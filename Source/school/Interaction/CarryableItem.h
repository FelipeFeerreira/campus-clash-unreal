#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarryableItem.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Item que o jogador pega, carrega (1 por vez) e solta. Genérico: o tipo e o valor
 * são dados (ex.: "Resistor", 4 Ω), então serve para outras aulas.
 */
UCLASS(Blueprintable)
class SCHOOL_API ACarryableItem : public AActor
{
    GENERATED_BODY()
public:
    ACarryableItem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTextRenderComponent> Label;

    /** Categoria usada nas receitas (ex.: Battery, Source, Resistor, Lamp, Wire). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FName ItemType;
    /** Valor numérico do item (V para fontes, Ω para resistores; 0 se não se aplica). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") float Value = 0.f;
    /** Nome mostrado na etiqueta, ex.: "Resistor 4 Ω". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FLinearColor Color = FLinearColor(.6f, .6f, .6f);

    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Holder;
    UFUNCTION(BlueprintPure) bool IsHeld() const { return Holder != nullptr; }

    /** Prende o item no carregador (sem física/colisão), na posição relativa dada. */
    void AttachTo(AActor* NewHolder, const FVector& RelativeOffset);
    /** Solta e deixa o item parado na posição dada (chão, bancada...). */
    void PlaceAt(const FVector& WorldLocation, const FRotator& WorldRotation = FRotator::ZeroRotator);
    /** Reaplica cor e etiqueta depois de mudar tipo/valor/nome. */
    UFUNCTION(BlueprintCallable) void RefreshAppearance();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;
};
