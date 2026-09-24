#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CircuitTypes.generated.h"

UENUM(BlueprintType)
enum class ECircuitPartType : uint8 { PowerSource, Resistor, Bulb };

UENUM(BlueprintType)
enum class ECircuitPhase : uint8 { Assembly, Passed, Burning, Cleared };

/** Deliberately a series circuit: the three sockets provide the wires and polarity. */
USTRUCT(BlueprintType)
struct SCHOOL_API FCircuitComponentData : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ECircuitPartType Type = ECircuitPartType::Resistor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Voltage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Resistance = 0;
};

USTRUCT(BlueprintType)
struct SCHOOL_API FCircuitResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bValid = false;
    UPROPERTY(BlueprintReadOnly) bool bPassed = false;
    UPROPERTY(BlueprintReadOnly) float Voltage = 0;
    UPROPERTY(BlueprintReadOnly) float Resistance = 0;
    UPROPERTY(BlueprintReadOnly) float Current = 0;
    UPROPERTY(BlueprintReadOnly) float Grade = 0;
    UPROPERTY(BlueprintReadOnly) FText Message;
};
