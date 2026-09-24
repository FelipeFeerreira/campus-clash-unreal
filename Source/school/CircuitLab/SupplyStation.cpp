#include "SupplyStation.h"
#include "Interaction/CarryableItem.h"
#include "Interaction/CarryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void TintMesh(UStaticMeshComponent* Mesh, const FLinearColor& Color)
    {
        if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CampusClash/Circuit/Materials/M_CircuitTint.M_CircuitTint")))
        {
            Mesh->SetMaterial(0, Base);
            if (auto* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
                Material->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
}

ASupplyStation::ASupplyStation()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

    // Balcão de 90 x 110 x 90 cm, apoiado no chão.
    Counter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Counter"));
    Counter->SetupAttachment(RootComponent);
    if (Cube.Succeeded()) Counter->SetStaticMesh(Cube.Object);
    Counter->SetRelativeScale3D(FVector(.9f, 1.1f, .9f));
    Counter->SetRelativeLocation(FVector(0, 0, 45));
    Counter->SetCollisionProfileName(TEXT("BlockAll"));

    // Amostra do item em cima do balcão, na cor do tipo.
    Sample = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sample"));
    Sample->SetupAttachment(RootComponent);
    if (Cube.Succeeded()) Sample->SetStaticMesh(Cube.Object);
    Sample->SetRelativeScale3D(FVector(.30f, .22f, .16f));
    Sample->SetRelativeLocation(FVector(0, 0, 98));
    Sample->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Placa virada para a frente da bancada (+X).
    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
    Sign->SetupAttachment(RootComponent);
    Sign->SetRelativeLocation(FVector(0, 0, 135));
    Sign->SetHorizontalAlignment(EHTA_Center);
    Sign->SetWorldSize(18);
    Sign->SetTextRenderColor(FColor::White);

    ItemName = FText::FromString(TEXT("Resistor"));
    SignText = FText::FromString(TEXT("RESISTORES"));
    ItemClass = ACarryableItem::StaticClass();
}

void ASupplyStation::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshAppearance();
}

void ASupplyStation::BeginPlay()
{
    Super::BeginPlay();
    RefreshAppearance();
}

void ASupplyStation::RefreshAppearance()
{
    Sign->SetText(SignText);
    TintMesh(Counter, FLinearColor(.12f, .14f, .17f));
    TintMesh(Sample, Color);
}

void ASupplyStation::Configure(FName InType, const FString& InName, const FString& InSign, const FString& InUnit, const TArray<float>& InValues, const FLinearColor& InColor)
{
    ItemType = InType;
    ItemName = FText::FromString(InName);
    SignText = FText::FromString(InSign);
    Unit = InUnit;
    Values = InValues;
    Color = InColor;
    RefreshAppearance();
}

FString ASupplyStation::FormatValue(float InValue, const FString& InUnit)
{
    if (InUnit.IsEmpty()) return FString();
    // Vírgula decimal (pt-BR) e sem zeros sobrando: 1,5 / 9 / 0,25.
    FString Number = FString::SanitizeFloat(InValue, 0).Replace(TEXT("."), TEXT(","));
    return Number + TEXT(" ") + InUnit;
}

ACarryableItem* ASupplyStation::SpawnItem()
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Item = World->SpawnActor<ACarryableItem>(ItemClass ? *ItemClass : ACarryableItem::StaticClass(), Sample->GetComponentLocation(), GetActorRotation(), Params);
    if (!Item) return nullptr;
    Item->ItemType = ItemType;
    Item->Value = Values.Num() > 0 ? Values[FMath::RandRange(0, Values.Num() - 1)] : 0.f;
    const FString ValueText = FormatValue(Item->Value, Unit);
    Item->DisplayName = FText::FromString(ValueText.IsEmpty() ? ItemName.ToString() : ItemName.ToString() + TEXT(" ") + ValueText);
    Item->Color = Color;
    Item->RefreshAppearance();
    return Item;
}

bool ASupplyStation::CanInteract(const APawn*, const ACarryableItem* Held) const
{
    // Mãos vazias pegam; o mesmo tipo pode ser devolvido; outro tipo não tem o que fazer aqui.
    return !Held || Held->ItemType == ItemType;
}

bool ASupplyStation::Interact(APawn*, UCarryComponent* Carry)
{
    if (!Carry) return false;
    if (Carry->IsHolding())
    {
        if (Carry->Held->ItemType != ItemType) return false;
        if (ACarryableItem* Returned = Carry->TakeHeld()) Returned->Destroy();
        return true;
    }
    ACarryableItem* Item = SpawnItem();
    if (!Item) return false;
    if (!Carry->PickUp(Item)) { Item->Destroy(); return false; }
    return true;
}

FText ASupplyStation::GetInteractPrompt(const ACarryableItem* Held) const
{
    return Held ? FText::Format(FText::FromString(TEXT("[E] Devolver {0}")), Held->DisplayName)
                : FText::Format(FText::FromString(TEXT("[E] Pegar {0}")), ItemName);
}
