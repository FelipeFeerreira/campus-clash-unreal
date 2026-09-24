#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CarryComponent.h"
#include "CarryableItem.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarryPickDropTest, "Campus.Interaction.CarryPickUpAndDrop", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarryPickDropTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Mundo de teste"), World)) return false;

    // "Jogador" simples: um ator com o componente (sem controller, o dono é o carregador).
    AActor* Player = World->SpawnActor<AActor>();
    Player->SetRootComponent(NewObject<USceneComponent>(Player, TEXT("Root")));
    Player->GetRootComponent()->RegisterComponent();
    auto* Carry = NewObject<UCarryComponent>(Player, TEXT("Carry"));
    Carry->RegisterComponent();

    auto* Near = World->SpawnActor<ACarryableItem>(FVector(100, 0, 0), FRotator::ZeroRotator);
    auto* Far = World->SpawnActor<ACarryableItem>(FVector(1000, 0, 0), FRotator::ZeroRotator);
    Near->ItemType = TEXT("Resistor"); Near->DisplayName = FText::FromString(TEXT("Resistor 4 ohm"));

    AActor* Target = nullptr;
    TestTrue(TEXT("Mãos vazias + item perto = pegar"), Carry->FindAction(Target) == ECarryAction::PickUp);
    TestEqual(TEXT("Alvo é o item mais próximo"), Target, (AActor*)Near);
    TestTrue(TEXT("E pega"), Carry->Interact());
    TestTrue(TEXT("Segurando"), Carry->IsHolding() && Near->IsHeld());
    TestEqual(TEXT("Item preso ao jogador"), Near->GetAttachParentActor(), Player);
    TestFalse(TEXT("Não pega um segundo item"), Carry->PickUp(Far));

    TestTrue(TEXT("Segurando e nada perto = soltar"), Carry->FindAction(Target) == ECarryAction::Drop);
    TestTrue(TEXT("Prompt de soltar"), Carry->GetPrompt().ToString().Contains(TEXT("Soltar")));
    TestTrue(TEXT("E solta"), Carry->Interact());
    TestFalse(TEXT("Mãos vazias"), Carry->IsHolding());
    TestFalse(TEXT("Item livre"), Near->IsHeld());
    TestNull(TEXT("Sem pai"), Near->GetAttachParentActor());
    TestTrue(TEXT("Caiu à frente do jogador"), FMath::IsNearlyEqual(Near->GetActorLocation().X, Carry->DropDistance, 1.f));

    TestTrue(TEXT("Pode pegar de novo"), Carry->Interact());
    TestTrue(TEXT("Segurando de novo"), Carry->IsHolding());

    World->DestroyWorld(false);
    return true;
}
#endif
