#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CircuitManager.h"
#include "CircuitActors.h"
#include "StudentProgress.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include <limits>

namespace CircuitTests
{
    TMap<FName, FCircuitComponentData> Series(float Voltage, float Resistance, float Bulb = 1)
    {
        TMap<FName, FCircuitComponentData> Parts;
        FCircuitComponentData Source; Source.Type=ECircuitPartType::PowerSource; Source.Voltage=Voltage;
        FCircuitComponentData R; R.Type=ECircuitPartType::Resistor; R.Resistance=Resistance;
        FCircuitComponentData Lamp; Lamp.Type=ECircuitPartType::Bulb; Lamp.Resistance=Bulb;
        Parts.Add(TEXT("Source"),Source); Parts.Add(TEXT("Resistor"),R); Parts.Add(TEXT("Bulb"),Lamp);
        return Parts;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircuitMathTest,"Campus.Circuit.SeriesAndValidation",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircuitMathTest::RunTest(const FString&)
{
    auto Result=UCircuitManager::Evaluate(CircuitTests::Series(10,9));
    TestTrue(TEXT("10V/(9+1)ohm passes"),Result.bPassed);
    TestEqual(TEXT("Current 1A"),Result.Current,1.f);
    TestEqual(TEXT("Grade 100"),Result.Grade,100.f);
    TestTrue(TEXT("Lower endpoint inclusive"),UCircuitManager::Evaluate(CircuitTests::Series(9,9)).bPassed);
    TestTrue(TEXT("Upper endpoint inclusive"),UCircuitManager::Evaluate(CircuitTests::Series(11,9)).bPassed);
    TestFalse(TEXT("Overcurrent fails"),UCircuitManager::Evaluate(CircuitTests::Series(10,4)).bPassed);
    TestFalse(TEXT("Undercurrent fails"),UCircuitManager::Evaluate(CircuitTests::Series(10,19)).bPassed);
    TestFalse(TEXT("Zero resistance rejected"),UCircuitManager::Evaluate(CircuitTests::Series(10,0,0)).bValid);
    TestFalse(TEXT("Negative resistance rejected"),UCircuitManager::Evaluate(CircuitTests::Series(10,-1)).bValid);
    TestFalse(TEXT("NaN rejected"),UCircuitManager::Evaluate(CircuitTests::Series(std::numeric_limits<float>::quiet_NaN(),9)).bValid);
    auto Missing=CircuitTests::Series(10,9); Missing.Remove(TEXT("Bulb"));
    TestFalse(TEXT("Missing lamp rejected"),UCircuitManager::Evaluate(Missing).bValid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircuitStateTest,"Campus.Circuit.StateAndReset",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircuitStateTest::RunTest(const FString&)
{
    auto* Manager=NewObject<UCircuitManager>();
    float Grade=123;
    TestFalse(TEXT("Incomplete cannot test"),Manager->CalculateVRI(Grade));
    TestEqual(TEXT("Incomplete clears output grade"),Grade,0.f);
    auto Parts=CircuitTests::Series(10,4);
    TestFalse(TEXT("Wrong slot type rejected"),Manager->AddComponent(TEXT("Source"),Parts[TEXT("Resistor")]));
    for(const auto& Pair:Parts) Manager->AddComponent(Pair.Key,Pair.Value);
    TestFalse(TEXT("Unsafe assembly fails"),Manager->CalculateVRI(Grade));
    TestTrue(TEXT("Failure starts burning"),Manager->Phase==ECircuitPhase::Burning);
    TestFalse(TEXT("Cannot reset while burning"),Manager->ResetCircuit());
    TestFalse(TEXT("Cannot remove while burning"),Manager->RemoveComponent(TEXT("Source")));
    TestFalse(TEXT("Cannot run repeated test"),Manager->CanRunTest());
    Manager->NotifyAllFiresCleared();
    TestTrue(TEXT("Cleared is separate from assembly"),Manager->Phase==ECircuitPhase::Cleared);
    TestTrue(TEXT("Recovery permits reset"),Manager->ResetCircuit());
    TestEqual(TEXT("Reset empties slots"),Manager->Slots.Num(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircuitProgressTest,"Campus.Circuit.PersistentRewardIdempotency",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircuitProgressTest::RunTest(const FString&)
{
    auto* Save=NewObject<UCampusCircuitSave>();
    Save->RecordGrade(TEXT("CircuitSeries01"),40);
    TestEqual(TEXT("Failure grants no reputation"),Save->Reputation,0);
    Save->RecordGrade(TEXT("CircuitSeries01"),80);
    Save->RecordGrade(TEXT("CircuitSeries01"),100);
    TestEqual(TEXT("Improvement does not duplicate reward"),Save->Reputation,2);
    TestTrue(TEXT("ElectroShock unlock persisted"),Save->bElectroShockUnlocked);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Serialize to memory"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    auto* Loaded=Cast<UCampusCircuitSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Reload save"),Loaded)) return false;
    Loaded->RecordGrade(TEXT("CircuitSeries01"),90);
    TestEqual(TEXT("Reload cannot farm reputation"),Loaded->Reputation,2);
    TestEqual(TEXT("Best grade does not decrease"),Loaded->BestGrades.FindRef(TEXT("CircuitSeries01")),100.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircuitActorFlowTest,"Campus.Circuit.WorkbenchFireRecovery",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircuitActorFlowTest::RunTest(const FString&)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("Test world"),World)) return false;
    auto* Bench=World->SpawnActor<ACircuitWorkbench>();
    auto* Door=World->SpawnActor<ADoorActor>();
    if(!Bench || !Door) { World->DestroyWorld(false); return false; }
    Bench->InitializeLesson();
    TestEqual(TEXT("Default catalog"),Bench->Components.Num(),5);
    AComponentBox* Source=nullptr; AComponentBox* Resistor=nullptr; AComponentBox* Bulb=nullptr;
    for(AComponentBox* Part:Bench->Components)
    {
        if(Part->Data.Type==ECircuitPartType::PowerSource) Source=Part;
        if(Part->Data.Type==ECircuitPartType::Bulb) Bulb=Part;
        if(Part->Data.Type==ECircuitPartType::Resistor && Part->Data.Resistance==4) Resistor=Part;
    }
    TestTrue(TEXT("Place source"),Bench->AddComponentToSlot(Source,TEXT("Source")));
    TestTrue(TEXT("Place wrong resistor"),Bench->AddComponentToSlot(Resistor,TEXT("Resistor")));
    TestTrue(TEXT("Place lamp"),Bench->AddComponentToSlot(Bulb,TEXT("Bulb")));
    Bench->RunTest();
    TestEqual(TEXT("Three fire targets"),Bench->Fires.Num(),3);
    TestTrue(TEXT("Door locked during emergency"),Door->bLocked);
    TestFalse(TEXT("Bench cannot reset emergency"),Bench->ResetCircuit());
    const auto Fires=Bench->Fires;
    if(Fires.Num()==3)
    {
        Fires[0]->Extinguish(2);
        TestEqual(TEXT("Individual target removed"),Bench->Fires.Num(),2);
        TestTrue(TEXT("Door stays locked until all cleared"),Door->bLocked);
        Fires[1]->Extinguish(2); Fires[2]->Extinguish(2);
    }
    TestTrue(TEXT("All fires transition to cleared"),Bench->Circuit->Phase==ECircuitPhase::Cleared);
    TestFalse(TEXT("Door unlocked after extinguishing"),Door->bLocked);
    TestTrue(TEXT("Reset succeeds"),Bench->ResetCircuit());
    TestEqual(TEXT("Reset clears slots"),Bench->Circuit->Slots.Num(),0);
    World->DestroyWorld(false);
    return true;
}
#endif
