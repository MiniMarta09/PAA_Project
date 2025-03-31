#pragma once

#include "GameHUD.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyPlayerController.h"
#include "BaseUnit.h"
#include "MyGameMode.generated.h"

UENUM()
enum class EPlacementTurn : uint8
{
    PlayerSniper,
    PlayerBrawler,
    AISniper,
    AIBrawler,
    Completed
};

UENUM(BlueprintType)
enum class EMovementTurn : uint8
{
    Player  UMETA(DisplayName = "Player"),
    AI      UMETA(DisplayName = "AI")
};

UCLASS()
class PAA_MARTA_API AMyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    AMyGameMode();

    virtual void BeginPlay() override;

    void OnCellHoverEnd(AGridCell* Cell);

    // Stato del posizionamento
    UPROPERTY(BlueprintReadOnly, Category = "Game State")
    EPlacementTurn CurrentPlacementTurn;

    UPROPERTY()
    class AGridManager* GridManager;

    // Classi per lo spawn delle unità
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Classes")
    TSubclassOf<class ABaseUnit> SniperClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Unit Classes")
    TSubclassOf<class ABaseUnit> BrawlerClass;

    UPROPERTY()
    ABaseUnit* SelectedUnitForMovement;

    // Turno corrente 
    UPROPERTY()
    EMovementTurn CurrentMovementTurn;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
    TSubclassOf<UGameHUD> HUDGameClass;

    // Riferimento all'istanza del widget HUD
    UPROPERTY()
    UGameHUD* HUD;

    // Funzionalità per il posizionamento
    UFUNCTION(BlueprintCallable, Category = "Game")
    void StartUnitPlacement();

    UFUNCTION(BlueprintCallable, Category = "Game")
    void OnGridCellClicked(class AGridCell* Cell);

    UFUNCTION(BlueprintCallable, Category = "Game")
    void AdvancePlacementTurn();

    UFUNCTION(BlueprintCallable, Category = "Game")
    void SpawnAndPlaceUnit(EUnitType UnitType, ETeamType TeamType, class AGridCell* Cell);

    // Funzione per posizionamento unità AI
    UFUNCTION(BlueprintCallable, Category = "Game")
    void PlaceAIUnit();

    UFUNCTION(BlueprintImplementableEvent, Category = "Game")
    void UpdatePlacementMessage(const FString& Message);

    UFUNCTION()
    void DisplayUnitRanges(ABaseUnit* SelectedUnit);

    UFUNCTION()
    void ResetAllCellHighlights();

    TSet<AGridCell*> GetReachableCells(ABaseUnit* Unit);

    UFUNCTION()
    void MoveAIUnits(); 

    UFUNCTION()
	void StartMovementPhase();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateMovementMessage(const FString& Message);

    UFUNCTION(BlueprintCallable, Category = "Movement")
    bool PlayerUnitsHaveMoved();

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void ResetPlayerUnitsMovement();

    void CheckWinCondition();

    void ProcessPendingCounterattacks();

    bool bGameOver;

	void MoveAIUnitsellHoverBegin(AGridCell* Cell); 

    bool PlayerUnitsHaveCompletedAction();

	void EndPlayerTurn();  

    FString GetCellIdentifier(AGridCell* Cell);

    void ResetAIUnitsMovement();


private:
    // Unità di anteprima da mostrare durante l'hover
    UPROPERTY()
    ABaseUnit* PreviewUnit;

    // Crea e posiziona un'unità di anteprima
    void CreateUnitPreview(EUnitType UnitType, ETeamType TeamType, AGridCell* Cell);

    // Rimuove l'unità di anteprima
    void DestroyUnitPreview();

    bool bAITurn;

    bool bAIStartsPlacement;
};