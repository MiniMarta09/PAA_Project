#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GridCell.generated.h"

class UStaticMeshComponent;

UCLASS()
class PAA_MARTA_API AGridCell : public AActor
{
    GENERATED_BODY()

public:

    AGridCell();

    // Flag per indicare se la cella è un ostacolo
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bIsObstacle;

    // Componente mesh per la cella
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    // Proprietà di occupazione
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    bool bIsOccupied;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    class ABaseUnit* OccupyingUnit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* NormalMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* ObstacleMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* HighlightMaterial;

    // Posizione nella griglia
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 GridY;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void InitializeCell(bool bInIsObstacle, const FVector& CellSize, UMaterialInterface* InNormalMaterial, UMaterialInterface* InObstacleMaterial);

    // Funzioni per la gestione dell'occupazione
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetOccupied(bool bOccupied);

    // Funzioni per la selezione 
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void OnCellClicked();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void OnCellHovered();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void OnCellUnhovered();

    UFUNCTION()
    void HighlightCell(const FLinearColor& Color);

    UFUNCTION()
    void ResetHighlight();

protected:

    virtual void BeginPlay() override;

private:

    UPROPERTY()
    UStaticMeshComponent* InfoBox;

    bool bIsInfoBoxVisible;
};