#include "CircuitManager.h"

bool UCircuitManager::Accepts(FName Slot, ECircuitPartType Type)
{
    return (Slot == TEXT("Source") && Type == ECircuitPartType::PowerSource)
        || (Slot == TEXT("Resistor") && Type == ECircuitPartType::Resistor)
        || (Slot == TEXT("Bulb") && Type == ECircuitPartType::Bulb);
}

bool UCircuitManager::AddComponent(FName Slot, const FCircuitComponentData& Data)
{
    if (Phase != ECircuitPhase::Assembly || !Accepts(Slot, Data.Type)
        || !FMath::IsFinite(Data.Voltage) || !FMath::IsFinite(Data.Resistance)
        || Data.Voltage < 0 || Data.Resistance < 0) return false;
    Slots.Add(Slot, Data);
    OnChanged.Broadcast();
    return true;
}

bool UCircuitManager::RemoveComponent(FName Slot)
{
    if (Phase != ECircuitPhase::Assembly || Slots.Remove(Slot) == 0) return false;
    OnChanged.Broadcast();
    return true;
}

bool UCircuitManager::AreAllSlotsFilled() const
{
    return Slots.Contains(TEXT("Source")) && Slots.Contains(TEXT("Resistor")) && Slots.Contains(TEXT("Bulb"));
}

bool UCircuitManager::CanRunTest() const { return Phase == ECircuitPhase::Assembly && AreAllSlotsFilled(); }

FCircuitResult UCircuitManager::Evaluate(const TMap<FName, FCircuitComponentData>& Components)
{
    FCircuitResult Result;
    Result.Message = FText::FromString(TEXT("Preencha fonte, resistor e lampada."));
    if (Components.Num() != 3) return Result;
    for (const FName Slot : {FName(TEXT("Source")), FName(TEXT("Resistor")), FName(TEXT("Bulb"))})
    {
        const auto* Data = Components.Find(Slot);
        if (!Data || !Accepts(Slot, Data->Type)) return Result;
        if (!FMath::IsFinite(Data->Voltage) || !FMath::IsFinite(Data->Resistance)
            || Data->Voltage < 0 || Data->Resistance < 0)
        {
            Result.Message = FText::FromString(TEXT("Valores invalidos. Use componentes do catalogo."));
            return Result;
        }
        if (Data->Type == ECircuitPartType::PowerSource) Result.Voltage += Data->Voltage;
        else Result.Resistance += Data->Resistance;
    }
    if (Result.Resistance <= KINDA_SMALL_NUMBER || Result.Voltage <= 0)
    {
        Result.Message = FText::FromString(TEXT("Circuito invalido: R deve ser maior que zero e a fonte deve estar ligada."));
        return Result;
    }
    Result.Current = Result.Voltage / Result.Resistance;
    if (!FMath::IsFinite(Result.Current)) return Result;
    Result.bValid = true;
    Result.bPassed = Result.Current >= 0.9f - KINDA_SMALL_NUMBER && Result.Current <= 1.1f + KINDA_SMALL_NUMBER;
    Result.Grade = Result.bPassed ? FMath::Clamp(100.f - FMath::Abs(Result.Current - 1.f) * 200.f, 80.f, 100.f) : 40.f;
    Result.Message = FText::FromString(Result.bPassed
        ? TEXT("Aprovado! Corrente dentro de 0,9 a 1,1 A.")
        : TEXT("Falha no teste simulado! Feche o painel, pegue o extintor e apague os 3 focos."));
    return Result;
}

bool UCircuitManager::CalculateVRI(float& OutGrade)
{
    OutGrade = 0;
    if (!CanRunTest()) return false;
    LastResult = Evaluate(Slots);
    OutGrade = LastResult.Grade;
    if (LastResult.bValid) Phase = LastResult.bPassed ? ECircuitPhase::Passed : ECircuitPhase::Burning;
    OnChanged.Broadcast();
    return LastResult.bPassed;
}

bool UCircuitManager::ResetCircuit()
{
    if (Phase == ECircuitPhase::Burning) return false;
    Slots.Reset();
    LastResult = FCircuitResult();
    Phase = ECircuitPhase::Assembly;
    OnChanged.Broadcast();
    return true;
}

void UCircuitManager::NotifyAllFiresCleared()
{
    if (Phase != ECircuitPhase::Burning) return;
    Phase = ECircuitPhase::Cleared;
    LastResult.Message = FText::FromString(TEXT("Bancada segura. Use Reiniciar circuito para tentar novamente."));
    OnChanged.Broadcast();
}
