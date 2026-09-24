#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CircuitTypes.h"
#include "CircuitActors.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UBoxComponent;
class UPointLightComponent;
class URadialForceComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UCircuitManager;
class ACircuitWorkbench;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDoorEntered, AActor*, Player);

UCLASS(Blueprintable)
class SCHOOL_API ADoorActor : public AActor
{
    GENERATED_BODY()
public:
    ADoorActor();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Pivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Trigger;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> DoorMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DestinationLevel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bLocked = false;
    UPROPERTY(BlueprintReadOnly) bool bOpen = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DoorTitle;
    UPROPERTY(BlueprintAssignable) FDoorEntered OnDoorEntered;
    UFUNCTION(BlueprintCallable) bool OpenDoor(AActor* Player);
    UFUNCTION(BlueprintCallable) void SetLocked(bool bValue);
protected:
    virtual void BeginPlay() override;
    UFUNCTION() void OnEntered(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bSweep, const FHitResult& Hit);
private:
    float OpenTime = 0;
};

UCLASS(Blueprintable)
class SCHOOL_API AComponentBox : public AActor
{
    GENERATED_BODY()
public:
    AComponentBox();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FCircuitComponentData Data;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ACircuitWorkbench> Bench;
    UPROPERTY(BlueprintReadOnly) FName PlacedSlot;
    UPROPERTY(BlueprintReadOnly) bool bGrabbed = false;
    FTransform HomeTransform;
    void RefreshAppearance();
    UFUNCTION(BlueprintCallable) bool Grab(AActor* Holder);
    UFUNCTION(BlueprintCallable) void Release();
};

UCLASS(Blueprintable)
class SCHOOL_API AExplosionActor : public AActor
{
    GENERATED_BODY()
public:
    AExplosionActor();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FireMesh;
    /** Área de acerto do extintor; tamanho fixo para o alvo não sumir enquanto o fogo diminui. */
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> HitBox;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> FireFX;
    UPROPERTY(VisibleAnywhere) TObjectPtr<URadialForceComponent> Force;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> FireSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> ExplosionSystem;
    UPROPERTY(BlueprintReadOnly) float FireIntensity = 1.f;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ACircuitWorkbench> Bench;
    UFUNCTION(BlueprintCallable) void Extinguish(float Amount);
protected:
    virtual void BeginPlay() override;
private:
    float Elapsed = 0;
};

UCLASS(Blueprintable)
class SCHOOL_API AExtinguisherActor : public AActor
{
    GENERATED_BODY()
public:
    AExtinguisherActor();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UNiagaraComponent> SprayFX;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> SpraySystem;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Holder;
    UPROPERTY(BlueprintReadOnly) bool bHitFire = false;
    UFUNCTION(BlueprintCallable) bool Grab(AActor* Player);
    UFUNCTION(BlueprintCallable) void Spray(bool bActive, float DeltaSeconds);
};

UCLASS(Blueprintable)
class SCHOOL_API ACircuitWorkbench : public AActor
{
    GENERATED_BODY()
public:
    ACircuitWorkbench();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> BenchMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> SlotSource;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> SlotResistor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> SlotBulb;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced) TObjectPtr<UCircuitManager> Circuit;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UDataTable> ComponentCatalog;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> FireSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> ExplosionSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> SpraySystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UNiagaraSystem> SuccessSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName LessonID = TEXT("CircuitSeries01");
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AComponentBox>> Components;
    UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AExplosionActor>> Fires;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AExtinguisherActor> Extinguisher;
    UPROPERTY(BlueprintReadOnly) bool bSaveWarning = false;
    UFUNCTION(BlueprintCallable) void InitializeLesson();
    UFUNCTION(BlueprintCallable) bool AddComponentToSlot(AComponentBox* Component, FName Slot);
    UFUNCTION(BlueprintCallable) bool RemoveFromSlot(FName Slot);
    UFUNCTION(BlueprintCallable) void RunTest();
    UFUNCTION(BlueprintCallable) bool ResetCircuit();
    UFUNCTION(BlueprintCallable) void FireCleared(AExplosionActor* Fire);
    UFUNCTION(BlueprintPure) FVector GetSlotPosition(FName Slot) const;
protected:
    virtual void BeginPlay() override;
private:
    void LockDoors(bool bLock);
};
