#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseUnit.generated.h"

UENUM(BlueprintType)
enum class EUnitType : uint8
{
    Sniper UMETA(DisplayName = "Sniper"),
    Brawler UMETA(DisplayName = "Brawler")
};

UENUM(BlueprintType)
enum class ETeamType : uint8
{
    Player UMETA(DisplayName = "Player"),
    AI UMETA(DisplayName = "AI")
};

UCLASS()
class PAA_MARTA_API ABaseUnit : public AActor
{
    GENERATED_BODY()

protected:

    virtual void BeginPlay() override;

public:

    ABaseUnit();

    static FString GetUnitDescription(const ABaseUnit* Unit);

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* UnitMesh;

    // Proprietà principali dell'unità
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    int32 MovementRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    FString AttackType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    int32 AttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    int32 MinDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    int32 MaxDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    int32 Health;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    int32 HealthMax;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    EUnitType UnitType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Properties")
    ETeamType TeamType;

    // Riferimento alla cella occupata
    UPROPERTY()
    class AGridCell* CurrentCell;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    bool bHasMoved = false;

    UPROPERTY()
    ABaseUnit* OccupyingUnit;

    // Funzionalità
    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    virtual void Initialize(EUnitType Type, ETeamType Team);

    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    virtual void PlaceOnGrid(class AGridCell* Cell);

    UFUNCTION()
    void OnUnitClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

    UFUNCTION(BlueprintCallable, Category = "Unit Movement")
    void MoveToCell(AGridCell* NewCell);

    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    void AttackTarget(ABaseUnit* Target);

    bool bHasAttacked;

    bool bPendingCounterattack;

    int32 PendingCounterDamage;

    TArray<AGridCell*> MovementPath;

    int32 CurrentPathIndex = 0;

    FTimerHandle MovementTimerHandle;

    float MovementStepDelay = 0.2f;

	void MoveStep();    

    TArray<AGridCell*>ComputePath(AGridCell* Start, AGridCell* Goal);

    void PerformDummyMove();
};