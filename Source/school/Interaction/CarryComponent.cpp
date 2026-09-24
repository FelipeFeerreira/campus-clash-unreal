#include "CarryComponent.h"
#include "CarryableItem.h"
#include "CampusInteractable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

AActor* UCarryComponent::GetCarrier() const
{
    if (const AController* Controller = Cast<AController>(GetOwner())) return Controller->GetPawn();
    return GetOwner();
}

bool UCarryComponent::PickUp(ACarryableItem* Item)
{
    AActor* Carrier = GetCarrier();
    if (!Carrier || !IsValid(Item) || Held || Item->IsHeld()) return false;
    Held = Item;
    Item->AttachTo(Carrier, HoldOffset);
    return true;
}

ACarryableItem* UCarryComponent::TakeHeld()
{
    ACarryableItem* Item = Held;
    Held = nullptr;
    if (IsValid(Item)) Item->PlaceAt(Item->GetActorLocation(), Item->GetActorRotation());
    return Item;
}

bool UCarryComponent::DropOnFloor()
{
    const AActor* Carrier = GetCarrier();
    if (!Held || !Carrier) return false;
    // Procura o chão à frente do personagem; sem chão (ex.: teste), usa a altura dos pés.
    const FVector Front = Carrier->GetActorLocation() + Carrier->GetActorForwardVector() * DropDistance;
    FVector Spot = Front - FVector(0, 0, Carrier->GetSimpleCollisionHalfHeight());
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CarryDrop), false, Carrier);
    Params.AddIgnoredActor(Held);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Front + FVector(0, 0, 50), Front - FVector(0, 0, 400), ECC_Visibility, Params))
        Spot = Hit.ImpactPoint;
    ACarryableItem* Item = TakeHeld();
    Item->PlaceAt(Spot, FRotator(0, Carrier->GetActorRotation().Yaw, 0));
    return true;
}

ECarryAction UCarryComponent::FindAction(AActor*& OutTarget) const
{
    OutTarget = nullptr;
    const AActor* Carrier = GetCarrier();
    if (!Carrier || !GetWorld()) return ECarryAction::None;
    const APawn* User = Cast<APawn>(Carrier);
    const FVector From = Carrier->GetActorLocation();
    float Best = FMath::Square(InteractRange);

    // 1) Estações/bancadas que aceitam a interação agora.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        const ICampusInteractable* Station = Cast<ICampusInteractable>(*It);
        if (!Station || !Station->CanInteract(User, Held)) continue;
        const float Dist = FVector::DistSquared2D(From, It->GetActorLocation());
        if (Dist < Best) { Best = Dist; OutTarget = *It; }
    }
    if (OutTarget) return ECarryAction::Station;

    // 2) Mãos vazias: item solto mais próximo.
    if (!Held)
    {
        for (TActorIterator<ACarryableItem> It(GetWorld()); It; ++It)
        {
            if (It->IsHeld()) continue;
            const float Dist = FVector::DistSquared2D(From, It->GetActorLocation());
            if (Dist < Best) { Best = Dist; OutTarget = *It; }
        }
        return OutTarget ? ECarryAction::PickUp : ECarryAction::None;
    }

    // 3) Segurando algo e nada por perto: soltar no chão.
    return ECarryAction::Drop;
}

bool UCarryComponent::Interact()
{
    AActor* Target = nullptr;
    switch (FindAction(Target))
    {
    case ECarryAction::Station:
        return Cast<ICampusInteractable>(Target)->Interact(Cast<APawn>(GetCarrier()), this);
    case ECarryAction::PickUp:
        return PickUp(Cast<ACarryableItem>(Target));
    case ECarryAction::Drop:
        return DropOnFloor();
    default:
        return false;
    }
}

FText UCarryComponent::GetPrompt() const
{
    AActor* Target = nullptr;
    switch (FindAction(Target))
    {
    case ECarryAction::Station:
        return Cast<ICampusInteractable>(Target)->GetInteractPrompt(Held);
    case ECarryAction::PickUp:
        return FText::Format(FText::FromString(TEXT("[E] Pegar {0}")), Cast<ACarryableItem>(Target)->DisplayName);
    case ECarryAction::Drop:
        return FText::Format(FText::FromString(TEXT("[E] Soltar {0} no chão")), Held->DisplayName);
    default:
        return FText::GetEmpty();
    }
}

float UCarryComponent::GetActionDistance() const
{
    AActor* Target = nullptr;
    const ECarryAction Action = FindAction(Target);
    const AActor* Carrier = GetCarrier();
    if (Action == ECarryAction::None || !Carrier) return TNumericLimits<float>::Max();
    // Soltar no chão é a opção de menor prioridade: nunca "ganha" de outra interação.
    if (Action == ECarryAction::Drop) return TNumericLimits<float>::Max() * .5f;
    return FVector::Dist2D(Carrier->GetActorLocation(), Target->GetActorLocation());
}
