#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CircuitTypes.h"
#include "CircuitManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCircuitChanged);

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class SCHOOL_API UCircuitManager : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly) TMap<FName, FCircuitComponentData> Slots;
    UPROPERTY(BlueprintReadOnly) ECircuitPhase Phase = ECircuitPhase::Assembly;
    UPROPERTY(BlueprintReadOnly) FCircuitResult LastResult;
    UPROPERTY(BlueprintAssignable) FCircuitChanged OnChanged;
    UFUNCTION(BlueprintCallable) bool AddComponent(FName Slot, const FCircuitComponentData& Data);
    UFUNCTION(BlueprintCallable) bool RemoveComponent(FName Slot);
    UFUNCTION(BlueprintPure) bool AreAllSlotsFilled() const;
    UFUNCTION(BlueprintPure) bool CanRunTest() const;
    UFUNCTION(BlueprintCallable) bool CalculateVRI(float& OutGrade);
    UFUNCTION(BlueprintCallable) bool ResetCircuit();
    UFUNCTION(BlueprintCallable) void NotifyAllFiresCleared();
    static bool Accepts(FName Slot, ECircuitPartType Type);
    static FCircuitResult Evaluate(const TMap<FName, FCircuitComponentData>& Components);
};
