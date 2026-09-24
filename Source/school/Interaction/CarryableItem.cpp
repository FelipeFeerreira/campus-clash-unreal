#include "CarryableItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

ACarryableItem::ACarryableItem()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
    // Cubo de 100 cm -> caixa de ~30 x 22 x 16 cm, apoiada na base (pivô no chão).
    Mesh->SetRelativeScale3D(FVector(.30f, .22f, .16f));
    Mesh->SetRelativeLocation(FVector(0, 0, 8));
    // Só o canal Visibility bloqueia (mira/traces); o jogador e a câmera atravessam.
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0, 0, 30));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(10);
    Label->SetTextRenderColor(FColor::White);
}

void ACarryableItem::BeginPlay()
{
    Super::BeginPlay();
    RefreshAppearance();
}

void ACarryableItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Etiqueta sempre virada para a câmera (só no eixo vertical), para ler de qualquer lado.
    if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
        if (PC->PlayerCameraManager)
        {
            const FVector ToCamera = PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation();
            Label->SetWorldRotation(FRotator(0, ToCamera.Rotation().Yaw, 0));
        }
}

void ACarryableItem::RefreshAppearance()
{
    Label->SetText(DisplayName.IsEmpty() ? FText::FromName(ItemType) : DisplayName);
    // Material com parâmetro "Color" (o mesmo do minigame de circuitos); sem ele, fica o padrão.
    if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CampusClash/Circuit/Materials/M_CircuitTint.M_CircuitTint")))
    {
        Mesh->SetMaterial(0, Base);
        if (auto* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
            Material->SetVectorParameterValue(TEXT("Color"), Color);
    }
}

void ACarryableItem::AttachTo(AActor* NewHolder, const FVector& RelativeOffset)
{
    Holder = NewHolder;
    AttachToActor(NewHolder, FAttachmentTransformRules::KeepWorldTransform);
    SetActorRelativeLocation(RelativeOffset);
    SetActorRelativeRotation(FRotator::ZeroRotator);
}

void ACarryableItem::PlaceAt(const FVector& WorldLocation, const FRotator& WorldRotation)
{
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Holder = nullptr;
    SetActorLocationAndRotation(WorldLocation, WorldRotation);
}
