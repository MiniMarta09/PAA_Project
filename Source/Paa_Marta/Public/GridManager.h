#pragma once

#include "Containers/Queue.h"
#include "GridCell.h"
#include "Containers/Set.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

class AGridCell;
struct FGridPosition
{
    int32 X;
    int32 Y;

    FGridPosition() : X(0), Y(0) {}
    FGridPosition(int32 InX, int32 InY) : X(InX), Y(InY) {}

    bool operator==(const FGridPosition& Other) const
    {
        return X == Other.X && Y == Other.Y;
    }

    friend uint32 GetTypeHash(const FGridPosition& GridPos)
    {
        return HashCombine(GetTypeHash(GridPos.X), GetTypeHash(GridPos.Y));
    }
};

UCLASS()
class PAA_MARTA_API AGridManager : public AActor
{
    GENERATED_BODY()

public:

    AGridManager();

    static AGridManager* GetInstance(UWorld* World);

    void InitializeGrid();

protected:

    virtual void BeginPlay() override;

public:
    // Array di celle
    UPROPERTY()
    TArray<class AGridCell*> GridCells;

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Grid")
    TSubclassOf<AGridCell> GridCellClass;

    // Dimensioni della griglia
    UPROPERTY(EditAnywhere, Category = "Grid")
    int32 GridRows = 25;

    UPROPERTY(EditAnywhere, Category = "Grid")
    int32 GridColumns = 25;

    // Dimensioni di ogni cella
    UPROPERTY(EditAnywhere, Category = "Grid")
    FVector CellSize = FVector(100.0f, 100.0f, 10.0f);

    // Percentuale di celle che saranno ostacoli
    UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float ObstaclePercentage = 20.0f;

    // Materiali per le celle 
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* NormalMaterial;

    // Materiali per gli ostacoli
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* ObstacleMaterial;

    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInterface* MountainMaterial;

    FORCEINLINE const TArray<AGridCell*>& GetGridCells() const { return GridCells; };

private:

    TArray<TArray<bool>> GridObstacles;

    // Funzioni per la generazione e controllo degli ostacoli
    void GenerateObstacles();
    bool IsGridFullyConnected();
    TSet<FGridPosition> FindConnectedCells(const FGridPosition& Start);

    static AGridManager* Instance;
    void EndPlay(const EEndPlayReason::Type EndPlayReason);
};