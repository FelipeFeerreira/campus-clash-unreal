#include "CircuitActors.h"
#include "CircuitManager.h"
#include "StudentProgress.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void SetCube(UStaticMeshComponent* Mesh)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
        Mesh->SetStaticMesh(Cube.Object);
    }
    void Tint(UStaticMeshComponent* Mesh, FLinearColor Color, float Glow = 0.f)
    {
        UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CampusClash/Circuit/Materials/M_CircuitTint.M_CircuitTint"));
        if (Base) Mesh->SetMaterial(0, Base);
        if (auto* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Material->SetScalarParameterValue(TEXT("Glow"), Glow);
        }
    }
    UTextRenderComponent* AddLabel(AActor* Actor, const TCHAR* Name, USceneComponent* Root)
    {
        auto* Text = Actor->CreateDefaultSubobject<UTextRenderComponent>(Name);
        Text->SetupAttachment(Root);
        Text->SetRelativeRotation(FRotator(0, -90, 0));
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetWorldSize(12);
        return Text;
    }
}

ADoorActor::ADoorActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
    Pivot->SetupAttachment(RootComponent);
    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
    DoorMesh->SetupAttachment(Pivot);
    SetCube(DoorMesh);
    DoorMesh->SetRelativeLocation(FVector(55, 0, 110));
    DoorMesh->SetRelativeScale3D(FVector(1.1, .12, 2.2));
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
    Trigger->SetupAttachment(RootComponent);
    Trigger->SetRelativeLocation(FVector(55, 0, 110));
    Trigger->SetBoxExtent(FVector(75, 100, 110));
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ADoorActor::OnEntered);
    Label = AddLabel(this, TEXT("DoorTitle"), RootComponent);
    Label->SetRelativeLocation(FVector(55, -15, 235));
    DoorTitle = FText::FromString(TEXT("Laboratorio de circuitos [E]"));
}

void ADoorActor::BeginPlay()
{
    Super::BeginPlay();
    Label->SetText(DoorTitle);
    Tint(DoorMesh, FLinearColor(.04, .35, .42));
}

bool ADoorActor::OpenDoor(AActor* Player)
{
    if (bLocked || bOpen || !IsValid(Player) || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) > FMath::Square(300.f)) return false;
    bOpen = true;
    OpenTime = 0;
    OnDoorEntered.Broadcast(Player);
    return true;
}

void ADoorActor::SetLocked(bool bValue) { bLocked = bValue; if (bValue) bOpen = false; }
void ADoorActor::OnEntered(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (bOpen && !bLocked && Cast<APawn>(Other)) OnDoorEntered.Broadcast(Other);
}

void ADoorActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Target = bOpen && !bLocked ? 95.f : 0.f;
    Pivot->SetRelativeRotation(FRotator(0, FMath::FInterpTo(Pivot->GetRelativeRotation().Yaw, Target, DeltaSeconds, 4), 0));
    if (bOpen && !bLocked && !DestinationLevel.IsNone())
    {
        OpenTime += DeltaSeconds;
        if (OpenTime >= .6f)
        {
            bOpen = false;
            UGameplayStatics::OpenLevel(this, DestinationLevel);
        }
    }
}

AComponentBox::AComponentBox()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ComponentMesh"));
    Mesh->SetupAttachment(RootComponent);
    SetCube(Mesh);
    Mesh->SetRelativeScale3D(FVector(.34, .28, .20));
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Label = AddLabel(this, TEXT("Values"), RootComponent);
    Label->SetRelativeLocation(FVector(0, -20, 20));
    Label->SetWorldSize(8);
}

void AComponentBox::RefreshAppearance()
{
    Label->SetText(Data.DisplayName);
    Tint(Mesh, Data.Type == ECircuitPartType::PowerSource ? FLinearColor(.15,.55,1)
        : Data.Type == ECircuitPartType::Bulb ? FLinearColor(1,.85,.1) : FLinearColor(.8,.25,.08));
}

bool AComponentBox::Grab(AActor* Holder)
{
    if (!Holder || !Bench || Bench->Circuit->Phase != ECircuitPhase::Assembly) return false;
    if (!PlacedSlot.IsNone() && !Bench->RemoveFromSlot(PlacedSlot)) return false;
    bGrabbed = true;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttachToActor(Holder, FAttachmentTransformRules::KeepWorldTransform);
    SetActorRelativeLocation(FVector(65, 0, 30));
    return true;
}

void AComponentBox::Release()
{
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    bGrabbed = false;
    PlacedSlot = NAME_None;
    SetActorTransform(HomeTransform);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

AExplosionActor::AExplosionActor()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    FireMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireTarget"));
    FireMesh->SetupAttachment(RootComponent);
    SetCube(FireMesh);
    FireMesh->SetRelativeScale3D(FVector(.35,.35,.6));
    // O visual encolhe com a intensidade; quem recebe o jato é a HitBox de tamanho fixo.
    FireMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FireHitBox"));
    HitBox->SetupAttachment(RootComponent);
    HitBox->SetBoxExtent(FVector(17.5f, 17.5f, 30.f)); // mesmo tamanho do cubo visual inicial
    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    FireFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireNiagara"));
    FireFX->SetupAttachment(RootComponent);
    FireFX->SetAutoActivate(false);
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
    Light->SetupAttachment(RootComponent);
    Light->SetLightColor(FLinearColor(1,.2,.01));
    Light->SetAttenuationRadius(160);
    Force = CreateDefaultSubobject<URadialForceComponent>(TEXT("Impulse"));
    Force->SetupAttachment(RootComponent);
    Force->Radius = 130;
    Force->ImpulseStrength = 80;
    Force->bIgnoreOwningActor = true;
    Force->bAutoActivate = false;
}

void AExplosionActor::BeginPlay()
{
    Super::BeginPlay();
    Tint(FireMesh, FLinearColor(1,.12,.01), 4);
    if (FireSystem) { FireFX->SetAsset(FireSystem); FireFX->Activate(); }
    if (ExplosionSystem) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionSystem, GetActorLocation());
    Force->FireImpulse(); // One impulse, never continuous force during recovery.
}

void AExplosionActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;
    const float Pulse = .85f + .15f * FMath::Sin(Elapsed * 11);
    FireMesh->SetRelativeScale3D(FVector(.35,.35,.6) * FMath::Max(.15f, FireIntensity) * Pulse);
    Light->SetIntensity(800 * FireIntensity * Pulse);
    FireFX->SetVariableFloat(TEXT("User.FireIntensity"), FireIntensity);
}

void AExplosionActor::Extinguish(float Amount)
{
    if (!FMath::IsFinite(Amount) || Amount <= 0 || FireIntensity <= 0) return;
    FireIntensity = FMath::Max(0.f, FireIntensity - Amount);
    if (FireIntensity <= KINDA_SMALL_NUMBER)
    {
        FireIntensity = 0;
        FireFX->Deactivate();
        Force->Deactivate();
        if (Bench) Bench->FireCleared(this);
        Destroy();
    }
}

AExtinguisherActor::AExtinguisherActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Extinguisher"));
    Mesh->SetupAttachment(RootComponent);
    SetCube(Mesh);
    Mesh->SetRelativeScale3D(FVector(.22,.22,.55));
    Label = AddLabel(this, TEXT("ExtinguisherLabel"), RootComponent);
    Label->SetRelativeLocation(FVector(0,0,45));
    Label->SetWorldSize(10);
    Label->SetText(FText::FromString(TEXT("EXTINTOR [E]")));
    SprayFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SprayNiagara"));
    SprayFX->SetupAttachment(RootComponent);
    SprayFX->SetRelativeLocation(FVector(20,0,20));
    SprayFX->SetAutoActivate(false);
}

bool AExtinguisherActor::Grab(AActor* Player)
{
    if (!Player || (Holder && Holder != Player)) return false;
    Holder = Player;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttachToActor(Player, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    SetActorRelativeLocation(FVector(30,35,15));
    Label->SetVisibility(false);
    Tint(Mesh, FLinearColor(.8,.025,.01));
    if (SpraySystem) SprayFX->SetAsset(SpraySystem);
    return true;
}

void AExtinguisherActor::Spray(bool bActive, float DeltaSeconds)
{
    bHitFire = false;
    APawn* Pawn = Cast<APawn>(Holder);
    auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!bActive || !PC) { SprayFX->Deactivate(); return; }
    if (!SprayFX->IsActive()) SprayFX->Activate();
    FVector Eye; FRotator Aim;
    PC->GetPlayerViewPoint(Eye, Aim);
    SprayFX->SetWorldRotation(Aim);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CircuitExtinguish), true, Pawn);
    Params.AddIgnoredActor(this);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Aim.Vector() * 800, ECC_Visibility, Params))
    {
        auto* Fire = Cast<AExplosionActor>(Hit.GetActor());
        if (Fire && FVector::DistSquared(GetActorLocation(), Hit.ImpactPoint) <= FMath::Square(500.f))
        {
            // Also reject camera peek-around-corner shots from a blocked nozzle.
            FHitResult Block;
            const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Block, SprayFX->GetComponentLocation(), Hit.ImpactPoint, ECC_Visibility, Params);
            if (!bBlocked || Block.GetActor() == Fire)
            {
                bHitFire = true;
                Fire->Extinguish(FMath::Clamp(DeltaSeconds, 0.f, .1f) * .55f);
            }
        }
    }
}

ACircuitWorkbench::ACircuitWorkbench()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    BenchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bench"));
    BenchMesh->SetupAttachment(RootComponent);
    SetCube(BenchMesh);
    BenchMesh->SetRelativeLocation(FVector(0,0,42));
    BenchMesh->SetRelativeScale3D(FVector(2.8,.85,.84));
    BenchMesh->SetCollisionProfileName(TEXT("BlockAll"));
    SlotSource = CreateDefaultSubobject<USceneComponent>(TEXT("Slot_Source"));
    SlotResistor = CreateDefaultSubobject<USceneComponent>(TEXT("Slot_Resistor"));
    SlotBulb = CreateDefaultSubobject<USceneComponent>(TEXT("Slot_Bulb"));
    SlotSource->SetupAttachment(RootComponent); SlotSource->SetRelativeLocation(FVector(-85,15,100));
    SlotResistor->SetupAttachment(RootComponent); SlotResistor->SetRelativeLocation(FVector(0,15,100));
    SlotBulb->SetupAttachment(RootComponent); SlotBulb->SetRelativeLocation(FVector(85,15,100));
    Label = AddLabel(this, TEXT("WorkbenchLabel"), RootComponent);
    Label->SetRelativeLocation(FVector(0,-45,130));
    Label->SetText(FText::FromString(TEXT("CIRCUITOS I - BANCADA [E]")));
    Circuit = CreateDefaultSubobject<UCircuitManager>(TEXT("CircuitManager"));
}

void ACircuitWorkbench::BeginPlay() { Super::BeginPlay(); InitializeLesson(); }

void ACircuitWorkbench::InitializeLesson()
{
    if (!Components.IsEmpty()) return;
    Tint(BenchMesh, FLinearColor(.035,.075,.1));
    TArray<FCircuitComponentData> Catalog;
    if (ComponentCatalog && ComponentCatalog->GetRowStruct() == FCircuitComponentData::StaticStruct())
    {
        TArray<FCircuitComponentData*> Rows;
        ComponentCatalog->GetAllRows(TEXT("CircuitCatalog"), Rows);
        for (const auto* Row : Rows) Catalog.Add(*Row);
    }
    if (Catalog.IsEmpty())
    {
        auto Add = [&Catalog](const TCHAR* Name, ECircuitPartType Type, float V, float R)
        { FCircuitComponentData D; D.DisplayName = FText::FromString(Name); D.Type = Type; D.Voltage = V; D.Resistance = R; Catalog.Add(D); };
        Add(TEXT("Fonte 10 V"), ECircuitPartType::PowerSource,10,0);
        Add(TEXT("Resistor 4 ohm"), ECircuitPartType::Resistor,0,4);
        Add(TEXT("Resistor 9 ohm"), ECircuitPartType::Resistor,0,9);
        Add(TEXT("Resistor 19 ohm"), ECircuitPartType::Resistor,0,19);
        Add(TEXT("Lampada 1 ohm"), ECircuitPartType::Bulb,0,1);
    }
    for (int32 Index = 0; Index < Catalog.Num(); ++Index)
    {
        const FVector Position = GetActorTransform().TransformPosition(FVector(-110 + Index * 50, -22, 100));
        auto* Part = GetWorld()->SpawnActor<AComponentBox>(Position, GetActorRotation());
        if (!Part) continue;
        Part->Data = Catalog[Index]; Part->Bench = this;
        Part->HomeTransform = Part->GetActorTransform();
        Part->RefreshAppearance();
        Components.Add(Part);
    }
    Extinguisher = GetWorld()->SpawnActor<AExtinguisherActor>(GetActorTransform().TransformPosition(FVector(175,0,35)), GetActorRotation());
    if (Extinguisher) Extinguisher->SpraySystem = SpraySystem;
}

FVector ACircuitWorkbench::GetSlotPosition(FName Slot) const
{
    return Slot == TEXT("Source") ? SlotSource->GetComponentLocation()
        : Slot == TEXT("Bulb") ? SlotBulb->GetComponentLocation() : SlotResistor->GetComponentLocation();
}

bool ACircuitWorkbench::AddComponentToSlot(AComponentBox* Component, FName Slot)
{
    if (!IsValid(Component) || Component->Bench != this || !Component->PlacedSlot.IsNone()
        || !Circuit->AddComponent(Slot, Component->Data)) return false;
    for (AComponentBox* Other : Components)
        if (Other && Other != Component && Other->PlacedSlot == Slot) Other->Release();
    Component->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Component->bGrabbed = false;
    Component->PlacedSlot = Slot;
    Component->SetActorLocation(GetSlotPosition(Slot));
    Component->Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    return true;
}

bool ACircuitWorkbench::RemoveFromSlot(FName Slot)
{
    if (!Circuit->RemoveComponent(Slot)) return false;
    for (AComponentBox* Part : Components) if (Part && Part->PlacedSlot == Slot) Part->Release();
    return true;
}

void ACircuitWorkbench::LockDoors(bool bLock)
{
    for (TActorIterator<ADoorActor> It(GetWorld()); It; ++It) It->SetLocked(bLock);
}

void ACircuitWorkbench::RunTest()
{
    if (!Circuit->CanRunTest()) return;
    float Grade = 0;
    const bool bPass = Circuit->CalculateVRI(Grade);
    if (!Circuit->LastResult.bValid) return;
    if (auto* GI = GetGameInstance())
        if (auto* Progress = GI->GetSubsystem<UStudentProgress>()) bSaveWarning = !Progress->AddGrade(LessonID, Grade);
    if (bPass)
    {
        if (SuccessSystem) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SuccessSystem, GetSlotPosition(TEXT("Bulb")));
        for (AComponentBox* Part : Components) if (Part && Part->PlacedSlot == TEXT("Bulb")) Tint(Part->Mesh, FLinearColor(1,.85,.1), 3);
    }
    else
    {
        LockDoors(true);
        for (FName Slot : {FName(TEXT("Source")),FName(TEXT("Resistor")),FName(TEXT("Bulb"))})
        {
            const FTransform Transform(GetActorRotation(), GetSlotPosition(Slot) + FVector(0,0,35));
            auto* Fire = GetWorld()->SpawnActorDeferred<AExplosionActor>(AExplosionActor::StaticClass(), Transform, this);
            if (!Fire) continue;
            Fire->Bench = this; Fire->FireSystem = FireSystem; Fire->ExplosionSystem = ExplosionSystem;
            Fires.Add(Fire);
            UGameplayStatics::FinishSpawningActor(Fire, Transform);
        }
        if (Fires.IsEmpty()) { Circuit->NotifyAllFiresCleared(); LockDoors(false); }
    }
    UE_LOG(LogTemp, Log, TEXT("Circuit %s: V=%.2f R=%.2f I=%.3f Grade=%.0f Phase=%d"), *LessonID.ToString(), Circuit->LastResult.Voltage, Circuit->LastResult.Resistance, Circuit->LastResult.Current, Grade, int32(Circuit->Phase));
}

bool ACircuitWorkbench::ResetCircuit()
{
    if (!Circuit->ResetCircuit()) return false;
    for (AComponentBox* Part : Components) if (Part) { Part->Release(); Part->RefreshAppearance(); }
    Fires.Empty();
    LockDoors(false);
    bSaveWarning = false;
    return true;
}

void ACircuitWorkbench::FireCleared(AExplosionActor* Fire)
{
    Fires.Remove(Fire);
    if (Fires.IsEmpty()) { Circuit->NotifyAllFiresCleared(); LockDoors(false); }
}
