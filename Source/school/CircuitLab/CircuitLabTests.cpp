#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "SupplyStation.h"
#include "Interaction/CarryComponent.h"
#include "Interaction/CarryableItem.h"
#include "Engine/World.h"

namespace CircuitLabTests
{
    /** Ator simples com UCarryComponent, fazendo papel de jogador. */
    UCarryComponent* MakePlayer(UWorld* World, const FVector& Location)
    {
        AActor* Player = World->SpawnActor<AActor>();
        Player->SetRootComponent(NewObject<USceneComponent>(Player, TEXT("Root")));
        Player->GetRootComponent()->RegisterComponent();
        Player->SetActorLocation(Location);
        auto* Carry = NewObject<UCarryComponent>(Player, TEXT("Carry"));
        Carry->RegisterComponent();
        return Carry;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSupplyStationTest, "Campus.CircuitLab.SupplyStation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSupplyStationTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Mundo de teste"), World)) return false;
    auto* Batteries = World->SpawnActor<ASupplyStation>(FVector(100, 0, 0), FRotator::ZeroRotator);
    Batteries->Configure(TEXT("Battery"), TEXT("Bateria"), TEXT("BATERIAS"), TEXT("V"), { 1.5f, 3.f, 9.f }, FLinearColor::Green);
    auto* Wires = World->SpawnActor<ASupplyStation>(FVector(-2000, 0, 0), FRotator::ZeroRotator);
    Wires->Configure(TEXT("Wire"), TEXT("Fio"), TEXT("FIOS"), TEXT(""), {}, FLinearColor::Red);
    UCarryComponent* Carry = CircuitLabTests::MakePlayer(World, FVector::ZeroVector);

    AActor* Target = nullptr;
    TestTrue(TEXT("Estação perto = interação de estação"), Carry->FindAction(Target) == ECarryAction::Station);
    TestTrue(TEXT("Prompt de pegar"), Carry->GetPrompt().ToString().Contains(TEXT("Pegar Bateria")));
    TestTrue(TEXT("E pega do estoque"), Carry->Interact());
    if (!TestTrue(TEXT("Segurando item novo"), Carry->IsHolding())) { World->DestroyWorld(false); return false; }
    TestEqual(TEXT("Tipo da estação"), Carry->Held->ItemType, FName(TEXT("Battery")));
    const float V = Carry->Held->Value;
    TestTrue(TEXT("Valor sorteado da lista"), V == 1.5f || V == 3.f || V == 9.f);
    TestTrue(TEXT("Nome com valor e unidade"), Carry->Held->DisplayName.ToString().StartsWith(TEXT("Bateria ")) && Carry->Held->DisplayName.ToString().EndsWith(TEXT(" V")));

    TestTrue(TEXT("Mesmo tipo pode devolver"), Carry->GetPrompt().ToString().Contains(TEXT("Devolver")));
    ACarryableItem* Given = Carry->Held;
    TestTrue(TEXT("E devolve"), Carry->Interact());
    TestFalse(TEXT("Mãos vazias depois de devolver"), Carry->IsHolding());
    TestTrue(TEXT("Item devolvido some"), !IsValid(Given) || Given->IsActorBeingDestroyed());

    // Outro tipo nas mãos: a estação de baterias não aceita.
    ACarryableItem* Wire = Wires->SpawnItem();
    TestEqual(TEXT("Fio sem valor mostra só o nome"), Wire->DisplayName.ToString(), FString(TEXT("Fio")));
    Carry->PickUp(Wire);
    TestFalse(TEXT("Estação recusa outro tipo"), Batteries->CanInteract(nullptr, Wire));
    TestTrue(TEXT("Com outro tipo, E solta no chão"), Carry->FindAction(Target) == ECarryAction::Drop);

    TestEqual(TEXT("Formato decimal com vírgula"), ASupplyStation::FormatValue(1.5f, TEXT("V")), FString(TEXT("1,5 V")));
    TestEqual(TEXT("Inteiro sem casas"), ASupplyStation::FormatValue(4.f, TEXT("ohm")), FString(TEXT("4 ohm")));
    World->DestroyWorld(false);
    return true;
}
#endif
