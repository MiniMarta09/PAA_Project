#include "MyGameMode.h"
#include "GridManager.h"
#include "GridCell.h"
#include "SniperUnit.h"
#include "BrawlerUnit.h"
#include "Kismet/GameplayStatics.h"

// Costruttore del GameMode: inizializza la classe HUD, il PlayerController e lo stato iniziale del posizionamento
AMyGameMode::AMyGameMode()
{
    static ConstructorHelpers::FClassFinder<UGameHUD> HUDGameBPClass(TEXT("/Game/BluePrint/GameBP"));
    if (HUDGameBPClass.Succeeded())
    {
        HUDGameClass = HUDGameBPClass.Class;
    }

    // Imposta la classe del PlayerController
    PlayerControllerClass = AMyPlayerController::StaticClass();

    // Stato iniziale: posizionamento dello Sniper del giocatore
    CurrentPlacementTurn = EPlacementTurn::PlayerSniper;

    bGameOver = false; // Partita in corso
    bAITurn = false;
}

// Funzione chiamata all'avvio del gioco: inizializza l'HUD, il GridManager e imposta l'ordine di posizionamento 
void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Inizializza l'HUD
    if (HUDGameClass)
    {
        HUD = CreateWidget<UGameHUD>(GetWorld(), HUDGameClass);
        if (HUD)
        {
            HUD->AddToViewport();
        }
    }

    // Inizializza il GridManager
    GridManager = AGridManager::GetInstance(GetWorld());
    if (!GridManager)
    {
        GridManager = GetWorld()->SpawnActor<AGridManager>(AGridManager::StaticClass(), FTransform::Identity);
        if (GridManager)
        {
            GridManager->InitializeGrid();
        }
    }

    // Imposta l'ordine di posizionamento in modo casuale e lo mostra nel widget 
    if (HUD)
    {
        // Ottiene il valore che indica se l'IA inizia il posizionamento
        bAIStartsPlacement = HUD->GetStartWithAI();
        if (bAIStartsPlacement)
        {
            // L'IA inizia: disabilita l'input, imposta il turno iniziale su AISniper e pianifica il posizionamento AI
            bAITurn = true;
            APlayerController* PC = GetWorld()->GetFirstPlayerController();
            if (PC)
            {
                PC->DisableInput(PC);
            }
            CurrentPlacementTurn = EPlacementTurn::AISniper;
            UpdateMovementMessage("AI has placed its Sniper");
            FTimerHandle TimerHandle;
            GetWorldTimerManager().SetTimer(TimerHandle, this, &AMyGameMode::PlaceAIUnit, 0.5f, false);
        }
        else
        {
            // Il giocatore inizia: input abilitato, turno iniziale su PlayerSniper
            bAITurn = false;
            CurrentPlacementTurn = EPlacementTurn::PlayerSniper;
            UpdateMovementMessage("Place your Sniper on the grid");
        }
    }
}

// Funzione chiamata quando il cursore inizia a passare sopra una cella (mostra l'anteprima) 
void AMyGameMode::MoveAIUnitsellHoverBegin(AGridCell* Cell)
{
    // Se la cella non è valida, è un ostacolo o è già occupata, non fa nulla
    if (!Cell || Cell->bIsObstacle || Cell->bIsOccupied)
    {
        return;
    }

    switch (CurrentPlacementTurn)
    {
    case EPlacementTurn::PlayerSniper:
        CreateUnitPreview(EUnitType::Sniper, ETeamType::Player, Cell);
        break;
    case EPlacementTurn::PlayerBrawler:
        CreateUnitPreview(EUnitType::Brawler, ETeamType::Player, Cell);
        break;
    default:
        break;
    }
}

// Funzione chiamata quando il cursore esce da una cella (elimina l'anteprima)
void AMyGameMode::OnCellHoverEnd(AGridCell* Cell)
{
    DestroyUnitPreview();
}

// Funzione chiamata al click su una cella della griglia
// Ad ogni turno, ogni unità, sia dell'IA che del giocatore, deve svolgere una delle seguenti azioni:
// 1. Muoversi 2. Attaccare 3. Muoversi e, se il range di attacco lo consente, anche attaccare
void AMyGameMode::OnGridCellClicked(AGridCell* Cell)
{
    if (bGameOver)
    {
        UE_LOG(LogTemp, Warning, TEXT("Game over - no actions allowed"));
        return;
    }

    if (bAITurn)
    {
        UE_LOG(LogTemp, Warning, TEXT("AI turn active - click ignored"));
        return;
    }

    if (!Cell)
    {
        UE_LOG(LogTemp, Warning, TEXT("Null cell clicked"));
        return;
    }

    if (Cell->bIsObstacle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot place unit on an obstacle"));
        return;
    }

    DestroyUnitPreview();

    if (bGameOver)
    {
        UE_LOG(LogTemp, Warning, TEXT("Game over - no actions allowed"));
        return;
    }

    // Fase di posizionamento
    if (CurrentPlacementTurn != EPlacementTurn::Completed)
    {
        switch (CurrentPlacementTurn)
        {
        case EPlacementTurn::PlayerSniper:
            SpawnAndPlaceUnit(EUnitType::Sniper, ETeamType::Player, Cell);
            HUD->SetExecutionText("HP: S", "Place", GetCellIdentifier(Cell));
            AdvancePlacementTurn();
            break;
        case EPlacementTurn::PlayerBrawler:
            SpawnAndPlaceUnit(EUnitType::Brawler, ETeamType::Player, Cell);
            HUD->SetExecutionText("HP: B", "Place", GetCellIdentifier(Cell));
            AdvancePlacementTurn();
            break;
        default:
            break;
        }
        return;
    }
    else // Fase di movimento/azione
    {
        if (CurrentMovementTurn == EMovementTurn::Player)
        {
            // Se nessuna unità è selezionata, tenta di selezionarne una presente nella cella
            if (!SelectedUnitForMovement)
            {
                if (Cell->OccupyingUnit && Cell->OccupyingUnit->TeamType == ETeamType::Player)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Selecting unit: %s"), *ABaseUnit::GetUnitDescription(Cell->OccupyingUnit));
                    DisplayUnitRanges(Cell->OccupyingUnit);
                    return;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("No unit selected and cell does not contain a player unit"));
                    return;
                }
            }

            if (Cell == SelectedUnitForMovement->CurrentCell)
            {
                UE_LOG(LogTemp, Warning, TEXT("Clicked on current cell; deselecting unit"));
                ResetAllCellHighlights();
                SelectedUnitForMovement = nullptr;
                return;
            }

            // Se la cella contiene un'unità nemica, tenta l'attacco
            if (Cell->OccupyingUnit && Cell->OccupyingUnit->TeamType != ETeamType::Player)
            {
                FString TargetCellID = GetCellIdentifier(Cell);
                UE_LOG(LogTemp, Warning, TEXT("Attacking enemy unit at cell %s"), *TargetCellID);
                SelectedUnitForMovement->AttackTarget(Cell->OccupyingUnit);
                SelectedUnitForMovement->bHasAttacked = true;

                FString UnitPrefix;
                if (SelectedUnitForMovement->UnitType == EUnitType::Sniper)
                    UnitPrefix = "HP: S";
                else if (SelectedUnitForMovement->UnitType == EUnitType::Brawler)
                    UnitPrefix = "HP: B";

                HUD->SetExecutionText(UnitPrefix, TargetCellID, FString::FromInt(5));
                // Dopo l'attacco, l'azione dell'unità è completata, non potrà muoversi
                SelectedUnitForMovement = nullptr;
            }
            else
            {
                // Se la cella è vuota, tenta il movimento (solo se l'unità non ha già mosso o attaccato)
                if (!SelectedUnitForMovement->bHasMoved && !SelectedUnitForMovement->bHasAttacked)
                {
                    TSet<AGridCell*> ReachableCells = GetReachableCells(SelectedUnitForMovement);
                    UE_LOG(LogTemp, Warning, TEXT("Reachable cells count: %d"), ReachableCells.Num());

                    if (ReachableCells.Contains(Cell))
                    {
                        FString Origin = GetCellIdentifier(SelectedUnitForMovement->CurrentCell);
                        UE_LOG(LogTemp, Warning, TEXT("Moving unit from %s to cell %s"), *Origin, *GetCellIdentifier(Cell));
                        SelectedUnitForMovement->MoveToCell(Cell);
                        SelectedUnitForMovement->bHasMoved = true;

                        FString UnitPrefix;
                        if (SelectedUnitForMovement->UnitType == EUnitType::Sniper)
                            UnitPrefix = "HP: S";
                        else if (SelectedUnitForMovement->UnitType == EUnitType::Brawler)
                            UnitPrefix = "HP: B";

                        HUD->SetExecutionText(UnitPrefix + " " + Origin, "->", GetCellIdentifier(Cell));
                        ResetAllCellHighlights();
                        // l'unità resta selezionata per permettere un eventuale attacco successivo
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Cell %s not reachable for movement"), *GetCellIdentifier(Cell));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Unit already attacked or moved; cannot move again."));
                }
            }
        }
    }
}

// Funzione che verifica se tutte le unità del giocatore hanno compiuto almeno un movimento o un attacco
bool AMyGameMode::PlayerUnitsHaveCompletedAction()
{
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);
    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->TeamType == ETeamType::Player && Unit->Health > 0)
        {
            // Se anche una sola unità non ha mosso o attaccato, il turno non è completato
            if (!(Unit->bHasMoved || Unit->bHasAttacked))
            {
                return false;
            }
        }
    }
    return true;
}

// Funzione che verifica se tutte le unità del giocatore hanno mosso
bool AMyGameMode::PlayerUnitsHaveMoved()
{
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);
    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->TeamType == ETeamType::Player)
        {
            // Se anche una sola unità non ha mosso, restituisce false
            if (!Unit->bHasMoved)
            {
                return false;
            }
        }
    }
    return true;
}

// Resetta i flag di movimento e attacco per tutte le unit del giocatore
void AMyGameMode::ResetPlayerUnitsMovement()
{
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);
    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->TeamType == ETeamType::Player)
        {
            Unit->bHasMoved = false;
            Unit->bHasAttacked = false;
        }
    }
}


// Resetta i flag di movimento e attacco per tutte le unit dell'AI
void AMyGameMode::ResetAIUnitsMovement()
{
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);
    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->TeamType == ETeamType::AI)
        {
            Unit->bHasMoved = false;
            Unit->bHasAttacked = false;
        }
    }
}

// Resetta i flag delle unit del giocatore e imposta il turno di movimento in base a chi ha iniziato il posizionamento
void AMyGameMode::StartMovementPhase()
{
    if (bGameOver)
        return;

    // Resetta i flag di movimento/attacco per tutte le unit del giocatore
    ResetPlayerUnitsMovement();
    ResetAIUnitsMovement();

    // Deseleziona qualsiasi unit precedentemente selezionata
    SelectedUnitForMovement = nullptr;

    // Se l'IA ha iniziato il posizionamento, l'IA muove per prima, se no il contrario
    if (bAIStartsPlacement)
    {
        CurrentMovementTurn = EMovementTurn::AI;
        MoveAIUnits();
    }
    else
    {
        CurrentMovementTurn = EMovementTurn::Player;
    }
}

void AMyGameMode::UpdateMovementMessage(const FString& Message)
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
    }
}

// Avanza il turno di posizionamento secondo l'ordine stabilito, in base a chi inizia (IA o giocatore)
void AMyGameMode::AdvancePlacementTurn()
{
    UE_LOG(LogTemp, Warning, TEXT("AdvancePlacementTurn called, current turn: %d"), (int)CurrentPlacementTurn);

    if (bAIStartsPlacement)
    {
        // Caso in cui inizia l'IA: AISniper -> PlayerSniper -> AIBrawler -> PlayerBrawler
        switch (CurrentPlacementTurn)
        {
        case EPlacementTurn::AISniper:
            CurrentPlacementTurn = EPlacementTurn::PlayerSniper;
            UpdateMovementMessage("Place your Sniper on the grid");
            {
                APlayerController* PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                {
                    PC->EnableInput(PC);
                }
            }
            bAITurn = false;
            break;
        case EPlacementTurn::PlayerSniper:
            CurrentPlacementTurn = EPlacementTurn::AIBrawler;
            UpdateMovementMessage("AI has placed its Brawler");
            {
                APlayerController* PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                {
                    PC->DisableInput(PC);
                }
            }
            bAITurn = true;
            PlaceAIUnit();
            break;
        case EPlacementTurn::AIBrawler:
            CurrentPlacementTurn = EPlacementTurn::PlayerBrawler;
            UpdateMovementMessage("Place your Brawler on the grid");
            {
                APlayerController* PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                {
                    PC->EnableInput(PC);
                }
            }
            bAITurn = false;
            break;
        case EPlacementTurn::PlayerBrawler:
            // Posizionamento completato
            CurrentPlacementTurn = EPlacementTurn::Completed;
            UpdateMovementMessage("Placement completed, The game can start!");
            if (HUD)
            {
                HUD->HideStartText();
            }
            break;
        default:
            break;
        }
    }
    else
    {
        // Caso in cui inizia il giocatore: PlayerSniper -> AISniper -> PlayerBrawler -> AIBrawler
        switch (CurrentPlacementTurn)
        {
        case EPlacementTurn::PlayerSniper:
            CurrentPlacementTurn = EPlacementTurn::AISniper;
            UpdateMovementMessage("AI has placed its Sniper");
            {
                APlayerController* PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                {
                    PC->DisableInput(PC);
                }
            }
            bAITurn = true;
            PlaceAIUnit();
            break;
        case EPlacementTurn::AISniper:
            CurrentPlacementTurn = EPlacementTurn::PlayerBrawler;
            UpdateMovementMessage("Place your Brawler on the grid");
            {
                APlayerController* PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                {
                    PC->EnableInput(PC);
                }
            }
            bAITurn = false;
            break;
        case EPlacementTurn::PlayerBrawler:
            CurrentPlacementTurn = EPlacementTurn::AIBrawler;
            UpdateMovementMessage("AI has placed its Brawler");
            {
                APlayerController* PC = GetWorld()->GetFirstPlayerController();
                if (PC)
                {
                    PC->DisableInput(PC);
                }
            }
            bAITurn = true;
            PlaceAIUnit();
            break;
        case EPlacementTurn::AIBrawler:
            CurrentPlacementTurn = EPlacementTurn::Completed;
            UpdateMovementMessage("Placement completed, The game can start!");
            if (HUD)
            {
                HUD->HideStartText();
            }
            break;
        default:
            break;
        }
    }

    if (CurrentPlacementTurn == EPlacementTurn::Completed)
    {
        // Al termine del posizionamento, riabilita l'input del giocatore e passa alla fase di movimento
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->EnableInput(PC);
        }
        bAITurn = false;
        StartMovementPhase();
    }
}

// Spawna e posiziona un'unità sulla cella specificata controllando se le condizioni di spawn sono valide
void AMyGameMode::SpawnAndPlaceUnit(EUnitType UnitType, ETeamType TeamType, AGridCell* Cell)
{
    if (!Cell || !GridManager || Cell->bIsOccupied || Cell->bIsObstacle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid spawn conditions"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    ABaseUnit* NewUnit = nullptr;
    FTransform SpawnTransform = Cell->GetActorTransform();

    if (UnitType == EUnitType::Sniper)
    {
        NewUnit = GetWorld()->SpawnActorDeferred<ASniperUnit>(ASniperUnit::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    }
    else if (UnitType == EUnitType::Brawler)
    {
        NewUnit = GetWorld()->SpawnActorDeferred<ABrawlerUnit>(ABrawlerUnit::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    }

    if (NewUnit)
    {
        NewUnit->Initialize(UnitType, TeamType);
        UGameplayStatics::FinishSpawningActor(NewUnit, SpawnTransform);
        NewUnit->PlaceOnGrid(Cell);
    }
}

// Funzione per il posizionamento IA
void AMyGameMode::PlaceAIUnit()
{
    UE_LOG(LogTemp, Warning, TEXT("PlaceAIUnit called"));

    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid or empty GridManager"));
        return;
    }

    TArray<AGridCell*> FreeCells;
    for (AGridCell* Cell : GridManager->GridCells)
    {
        if (Cell && !Cell->bIsOccupied && !Cell->bIsObstacle)
        {
            FreeCells.Add(Cell);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("Free cells found: %d"), FreeCells.Num());

    if (FreeCells.Num() > 0)
    {
        int32 RandomIndex = FMath::RandRange(0, FreeCells.Num() - 1);
        AGridCell* SelectedCell = FreeCells[RandomIndex];
        UE_LOG(LogTemp, Warning, TEXT("AI selects cell: %s"), *SelectedCell->GetName());

        if (CurrentPlacementTurn == EPlacementTurn::AISniper)
        {
            SpawnAndPlaceUnit(EUnitType::Sniper, ETeamType::AI, SelectedCell);
            if (HUD)
            {
                HUD->SetExecutionText("AI: S", "Place", GetCellIdentifier(SelectedCell));
            }
        }
        else if (CurrentPlacementTurn == EPlacementTurn::AIBrawler)
        {
            SpawnAndPlaceUnit(EUnitType::Brawler, ETeamType::AI, SelectedCell);
            if (HUD)
            {
                HUD->SetExecutionText("AI: B", "Place", GetCellIdentifier(SelectedCell));
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("Directly calling AdvancePlacementTurn"));
        AdvancePlacementTurn();
    }
    else
    {
        AdvancePlacementTurn();
    }
}

void AMyGameMode::CreateUnitPreview(EUnitType UnitType, ETeamType TeamType, AGridCell* Cell)
{
    if (!Cell)
    {
        return;
    }

    // Elimina eventuale anteprima esistente
    DestroyUnitPreview();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Spawna l'unità di anteprima in base al tipo
    if (UnitType == EUnitType::Sniper)
    {
        PreviewUnit = GetWorld()->SpawnActor<ASniperUnit>(ASniperUnit::StaticClass(), Cell->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
    }
    else if (UnitType == EUnitType::Brawler)
    {
        PreviewUnit = GetWorld()->SpawnActor<ABrawlerUnit>(ABrawlerUnit::StaticClass(), Cell->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
    }

    if (PreviewUnit)
    {
        PreviewUnit->Initialize(UnitType, TeamType);

        // Disabilita collisioni sull'anteprima 
        if (PreviewUnit->UnitMesh)
        {
            PreviewUnit->UnitMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            for (int32 i = 0; i < PreviewUnit->UnitMesh->GetNumMaterials(); i++)
            {
                UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(
                    PreviewUnit->UnitMesh->GetMaterial(i), nullptr);
                if (DynMaterial)
                {
                    DynMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.5f);
                    PreviewUnit->UnitMesh->SetMaterial(i, DynMaterial);
                }
            }
        }

        // Posiziona l'anteprima appena sopra la cella 
        FVector CellLocation = Cell->GetActorLocation();
        PreviewUnit->SetActorLocation(FVector(CellLocation.X, CellLocation.Y, CellLocation.Z + 50.0f));
    }
}

void AMyGameMode::DestroyUnitPreview()
{
    if (PreviewUnit)
    {
        PreviewUnit->Destroy();
        PreviewUnit = nullptr;
    }
}

// Inizia la fase di posizionamento delle unit impostando il turno iniziale
void AMyGameMode::StartUnitPlacement()
{
    CurrentPlacementTurn = EPlacementTurn::PlayerSniper;
    UpdatePlacementMessage("Place your Sniper on the grid");
}

// Visualizza i range di movimento e attacco per l'unit selezionata
void AMyGameMode::DisplayUnitRanges(ABaseUnit* SelectedUnit)
{
    if (!SelectedUnit || SelectedUnit->TeamType != ETeamType::Player || !SelectedUnit->CurrentCell || !GridManager)
    {
        return;
    }

    // Se l'unit ha già attaccato, non mostra alcun range
    if (SelectedUnit->bHasAttacked)
    {
        UE_LOG(LogTemp, Warning, TEXT("This unit has already attacked this turn"));
        return;
    }

    ResetAllCellHighlights();

    int32 OriginX = SelectedUnit->CurrentCell->GridX;
    int32 OriginY = SelectedUnit->CurrentCell->GridY;

    // Calcola il range di movimento 
    TSet<AGridCell*> ReachableCells;
    if (!SelectedUnit->bHasMoved)
    {
        ReachableCells = GetReachableCells(SelectedUnit);
    }

    // Calcola l'insieme delle celle nel range d'attacco basato sulla distanza Manhattan
    TSet<AGridCell*> AttackCells;
    for (AGridCell* Cell : GridManager->GridCells)
    {
        if (!Cell || Cell->bIsObstacle)
            continue;
        int32 DeltaX = FMath::Abs(Cell->GridX - OriginX);
        int32 DeltaY = FMath::Abs(Cell->GridY - OriginY);
        int32 ManhattanDistance = DeltaX + DeltaY;
        if (ManhattanDistance <= SelectedUnit->AttackRange)
        {
            AttackCells.Add(Cell);
        }
    }

    // Definisce i colori per evidenziare il range di movimento e attacco
    FLinearColor MovementRangeColor(1.0f, 0.2f, 0.6f, 0.8f); // Rosa 
    FLinearColor AttackRangeColor(0.1f, 0.0f, 0.1f, 0.8f);    // Viola 

    if (SelectedUnit->AttackType.Equals(TEXT("Ranged Attack")))
    {
        if (!SelectedUnit->bHasMoved)
        {
            for (AGridCell* Cell : ReachableCells)
            {
                Cell->HighlightCell(MovementRangeColor);
            }
            for (AGridCell* Cell : AttackCells)
            {
                if (!ReachableCells.Contains(Cell))
                {
                    Cell->HighlightCell(AttackRangeColor);
                }
            }
        }
        else
        {
            // Se l'unità ha mosso, evidenzia solo il range d'attacco perchè una volta mosso, l'unità potrebbe ancora attaccare
            for (AGridCell* Cell : AttackCells)
            {
                Cell->HighlightCell(AttackRangeColor);
            }
        }
    }
    else
    {
        if (!SelectedUnit->bHasMoved)
        {
            for (AGridCell* Cell : GridManager->GridCells)
            {
                if (!Cell || Cell->bIsObstacle)
                    continue;
                int32 DeltaX = FMath::Abs(Cell->GridX - OriginX);
                int32 DeltaY = FMath::Abs(Cell->GridY - OriginY);
                int32 ManhattanDistance = DeltaX + DeltaY;
                if (ManhattanDistance == 1)
                {
                    Cell->HighlightCell(AttackRangeColor);
                }
                else if (ReachableCells.Contains(Cell))
                {
                    Cell->HighlightCell(MovementRangeColor);
                }
            }
        }
        else
        {
            for (AGridCell* Cell : GridManager->GridCells)
            {
                if (!Cell || Cell->bIsObstacle)
                    continue;
                int32 DeltaX = FMath::Abs(Cell->GridX - OriginX);
                int32 DeltaY = FMath::Abs(Cell->GridY - OriginY);
                int32 ManhattanDistance = DeltaX + DeltaY;
                if (ManhattanDistance == 1)
                {
                    Cell->HighlightCell(AttackRangeColor);
                }
            }
        }
    }

    // Mantiene l'unità selezionata per consentire l'attacco
    SelectedUnitForMovement = SelectedUnit;
    HUD->SetHealthBar(SelectedUnit->HealthMax, SelectedUnit->Health);
}

// Resetta l'evidenziazione di tutte le celle della griglia
void AMyGameMode::ResetAllCellHighlights()
{
    if (!GridManager)
    {
        return;
    }
    for (AGridCell* Cell : GridManager->GridCells)
    {
        if (Cell)
        {
            Cell->ResetHighlight();
        }
    }
}

// Calcola le celle raggiungibili da un'unità usando la BFS
TSet<AGridCell*> AMyGameMode::GetReachableCells(ABaseUnit* Unit)
{
    TSet<AGridCell*> Reachable;
    if (!Unit || !Unit->CurrentCell || !GridManager) return Reachable;

    const int32 MaxRange = Unit->MovementRange;
    TQueue<TPair<AGridCell*, int32>> Queue;
    TSet<AGridCell*> Visited;

    Queue.Enqueue(TPair<AGridCell*, int32>(Unit->CurrentCell, 0));
    Visited.Add(Unit->CurrentCell);

    while (!Queue.IsEmpty())
    {
        TPair<AGridCell*, int32> CurrentPair;
        Queue.Dequeue(CurrentPair);
        AGridCell* CurrentCell = CurrentPair.Key;
        int32 Distance = CurrentPair.Value;

        if (Distance > 0)
        {
            Reachable.Add(CurrentCell);
        }

        if (Distance < MaxRange)
        {
            TArray<AGridCell*> Neighbors;
            for (AGridCell* Cell : GridManager->GridCells)
            {
                if (Cell)
                {
                    if ((Cell->GridX == CurrentCell->GridX + 1 && Cell->GridY == CurrentCell->GridY) ||
                        (Cell->GridX == CurrentCell->GridX - 1 && Cell->GridY == CurrentCell->GridY) ||
                        (Cell->GridX == CurrentCell->GridX && Cell->GridY == CurrentCell->GridY + 1) ||
                        (Cell->GridX == CurrentCell->GridX && Cell->GridY == CurrentCell->GridY - 1))
                    {
                        Neighbors.Add(Cell);
                    }
                }
            }

            for (AGridCell* Neighbor : Neighbors)
            {
                if (!Visited.Contains(Neighbor))
                {
                    if (!Neighbor->bIsObstacle && (!Neighbor->bIsOccupied || Neighbor == Unit->CurrentCell))
                    {
                        Visited.Add(Neighbor);
                        Queue.Enqueue(TPair<AGridCell*, int32>(Neighbor, Distance + 1));
                    }
                }
            }
        }
    }
    return Reachable;
}

// Gestisce le azioni dell'IA durante il turno di movimento/azione
void AMyGameMode::MoveAIUnits()
{
    ResetAIUnitsMovement();
    if (bGameOver) { return; }

    ProcessPendingCounterattacks();

    TArray<AActor*> PlayerUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), PlayerUnits);
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);

    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* AIUnit = Cast<ABaseUnit>(Actor);
        if (AIUnit && AIUnit->TeamType == ETeamType::AI && AIUnit->Health > 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("AIUnit: %s, Type: %d"), *ABaseUnit::GetUnitDescription(AIUnit), (int32)AIUnit->UnitType);

            bool bHasActed = false;

            // Prova subito ad attaccare prima di muoversi
            for (AActor* PActor : PlayerUnits)
            {
                ABaseUnit* PlayerUnit = Cast<ABaseUnit>(PActor);
                if (PlayerUnit && PlayerUnit->TeamType == ETeamType::Player && PlayerUnit->Health > 0)
                {
                    int32 DeltaX = FMath::Abs(AIUnit->CurrentCell->GridX - PlayerUnit->CurrentCell->GridX);
                    int32 DeltaY = FMath::Abs(AIUnit->CurrentCell->GridY - PlayerUnit->CurrentCell->GridY);
                    int32 ManhattanDistance = DeltaX + DeltaY;

                    bool bInRange = (AIUnit->AttackType.Equals(TEXT("Ranged Attack")))
                        ? (ManhattanDistance <= AIUnit->AttackRange)
                        : (ManhattanDistance == 1);

                    if (bInRange)
                    {
                        FString TargetCellID = GetCellIdentifier(PlayerUnit->CurrentCell);
                        AIUnit->AttackTarget(PlayerUnit);
                        AIUnit->bHasAttacked = true;
                        int32 Damage = 4;

                        FString AIUnitPrefix = (AIUnit->UnitType == EUnitType::Sniper) ? "AI: S" : "AI: B";

                        if (HUD)
                        {
                            HUD->SetExecutionText(AIUnitPrefix, TargetCellID, FString::FromInt(Damage));
                        }
                        UE_LOG(LogTemp, Warning, TEXT("%s attacks %s at cell %s causing %d damage"),
                            *ABaseUnit::GetUnitDescription(AIUnit),
                            *ABaseUnit::GetUnitDescription(PlayerUnit),
                            *TargetCellID, Damage);

                        CheckWinCondition();
                        bHasActed = true;
                        break;
                    }
                }
            }

            if (bHasActed) continue;  // se ha già attaccato passa alla prossima unità IA

            // Movimento verso il giocatore più vicino
            TSet<AGridCell*> ReachableCells = GetReachableCells(AIUnit);
            TArray<AGridCell*> ReachableArray = ReachableCells.Array();

            if (ReachableArray.Num() > 0)
            {
                ABaseUnit* NearestPlayer = nullptr;
                int32 MinDistance = TNumericLimits<int32>::Max();

                // Trova il giocatore più vicino
                for (AActor* PActor : PlayerUnits)
                {
                    ABaseUnit* PlayerUnit = Cast<ABaseUnit>(PActor);
                    if (PlayerUnit && PlayerUnit->TeamType == ETeamType::Player && PlayerUnit->Health > 0)
                    {
                        int32 Distance = FMath::Abs(AIUnit->CurrentCell->GridX - PlayerUnit->CurrentCell->GridX) +
                            FMath::Abs(AIUnit->CurrentCell->GridY - PlayerUnit->CurrentCell->GridY);
                        if (Distance < MinDistance)
                        {
                            MinDistance = Distance;
                            NearestPlayer = PlayerUnit;
                        }
                    }
                }

                AGridCell* TargetCell = nullptr;
                int32 BestDistance = TNumericLimits<int32>::Max();

                // Seleziona cella migliore verso il giocatore
                if (NearestPlayer)
                {
                    for (AGridCell* Cell : ReachableArray)
                    {
                        int32 DistanceToTarget = FMath::Abs(Cell->GridX - NearestPlayer->CurrentCell->GridX) +
                            FMath::Abs(Cell->GridY - NearestPlayer->CurrentCell->GridY);
                        if (DistanceToTarget < BestDistance)
                        {
                            BestDistance = DistanceToTarget;
                            TargetCell = Cell;
                        }
                    }
                }

                if (TargetCell)
                {
                    FString Origin = GetCellIdentifier(AIUnit->CurrentCell);
                    AIUnit->MoveToCell(TargetCell);
                    AIUnit->bHasMoved = true;

                    // Aggiorna subito la cella corrente
                    AIUnit->CurrentCell = TargetCell;

                    FString AIUnitPrefix = (AIUnit->UnitType == EUnitType::Sniper) ? "AI: S" : "AI: B";

                    if (HUD)
                    {
                        HUD->SetExecutionText(AIUnitPrefix + " " + Origin, "->", GetCellIdentifier(TargetCell));
                    }

                    UE_LOG(LogTemp, Warning, TEXT("%s moved from cell %s to %s"),
                        *ABaseUnit::GetUnitDescription(AIUnit),
                        *Origin, *GetCellIdentifier(TargetCell));

                    // Dopo il movimento verifica di nuovo se può attaccare
                    for (AActor* PActor : PlayerUnits)
                    {
                        ABaseUnit* PlayerUnit = Cast<ABaseUnit>(PActor);
                        if (PlayerUnit && PlayerUnit->TeamType == ETeamType::Player && PlayerUnit->Health > 0)
                        {
                            int32 DeltaX = FMath::Abs(AIUnit->CurrentCell->GridX - PlayerUnit->CurrentCell->GridX);
                            int32 DeltaY = FMath::Abs(AIUnit->CurrentCell->GridY - PlayerUnit->CurrentCell->GridY);
                            int32 ManhattanDistance = DeltaX + DeltaY;

                            bool bInRange = (AIUnit->AttackType.Equals(TEXT("Ranged Attack")))
                                ? (ManhattanDistance <= AIUnit->AttackRange)
                                : (ManhattanDistance == 1);

                            if (bInRange)
                            {
                                FString TargetCellID = GetCellIdentifier(PlayerUnit->CurrentCell);
                                AIUnit->AttackTarget(PlayerUnit);
                                AIUnit->bHasAttacked = true;
                                int32 Damage = 4;

                                if (HUD)
                                {
                                    HUD->SetExecutionText(AIUnitPrefix, TargetCellID, FString::FromInt(Damage));
                                }
                                UE_LOG(LogTemp, Warning, TEXT("%s attacks %s at cell %s causing %d damage AFTER MOVING"),
                                    *ABaseUnit::GetUnitDescription(AIUnit),
                                    *ABaseUnit::GetUnitDescription(PlayerUnit),
                                    *TargetCellID, Damage);

                                CheckWinCondition();
                                break;
                            }
                        }
                    }
                }
            }

            // Se non può né muovere né attaccare fa un movimento dummy
            if (!AIUnit->bHasAttacked && !AIUnit->bHasMoved)
            {
                AIUnit->PerformDummyMove();
                UE_LOG(LogTemp, Warning, TEXT("%s performs a dummy move as fallback"), *ABaseUnit::GetUnitDescription(AIUnit));
            }
        }
    }

    if (!bGameOver)
    {
        CurrentMovementTurn = EMovementTurn::Player;
        UpdateMovementMessage(TEXT("Player Turn: It's your turn to move or attack"));
    }
}

// Verifica la condizione di vittoria controllando se entrambe le unit di una squadra sono state eliminate
void AMyGameMode::CheckWinCondition()
{
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);

    bool bPlayerUnitsExist = false;
    bool bAIUnitsExist = false;

    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->Health > 0)
        {
            if (Unit->TeamType == ETeamType::Player)
                bPlayerUnitsExist = true;
            else if (Unit->TeamType == ETeamType::AI)
                bAIUnitsExist = true;
        }
    }

    if (!bAIUnitsExist || !bPlayerUnitsExist)
    {
        bGameOver = true;

        // Disabilita l'input del giocatore
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            PC->DisableInput(PC);
        }

        // Resetta evidenziazioni e selezioni
        SelectedUnitForMovement = nullptr;
        ResetAllCellHighlights();

        // Mostra il messaggio di vittoria/sconfitta 
        if (HUD)
        {
            HUD->ShowEndGameMessage(!bAIUnitsExist); // true se vince il giocatore, false se vince AI
        }
    }
}

void AMyGameMode::ProcessPendingCounterattacks()
{
    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseUnit::StaticClass(), FoundUnits);
    for (AActor* Actor : FoundUnits)
    {
        ABaseUnit* Unit = Cast<ABaseUnit>(Actor);
        if (Unit && Unit->TeamType == ETeamType::Player && Unit->bPendingCounterattack)
        {
            Unit->Health -= Unit->PendingCounterDamage;
            UE_LOG(LogTemp, Warning, TEXT("Counterattack! %s suffers %d damage. Remaining health: %d"),
                *ABaseUnit::GetUnitDescription(Unit), Unit->PendingCounterDamage, Unit->Health);

            AMyGameMode* GM = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
            if (GM && GM->HUD)
            {
                GM->HUD->SetHealthBar(Unit->HealthMax, Unit->Health);
            }

            if (GEngine)
            {
                FString CounterMsg = FString::Printf(TEXT("Counterattack! %s: -%d damage, remaining health %d"),
                    *ABaseUnit::GetUnitDescription(Unit),
                    Unit->PendingCounterDamage, Unit->Health);
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, CounterMsg);
            }
            Unit->bPendingCounterattack = false;
            Unit->PendingCounterDamage = 0;

            if (Unit->Health <= 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("%s has been eliminated by counterattack!"),
                    *ABaseUnit::GetUnitDescription(Unit));
                if (GEngine)
                {
                    FString DeathMsg = FString::Printf(TEXT("%s has been eliminated by counterattack!"),
                        *ABaseUnit::GetUnitDescription(Unit));
                    GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, DeathMsg);
                }
                Unit->Destroy();

                CheckWinCondition();
            }
        }
    }
}

void AMyGameMode::EndPlayerTurn()
{
    if (!PlayerUnitsHaveCompletedAction())
    {
        UE_LOG(LogTemp, Warning, TEXT("Not all units have acted!"));
        return;
    }
    ResetPlayerUnitsMovement();
    ResetAllCellHighlights();

    CheckWinCondition();
    if (bGameOver)
    {
        UpdateMovementMessage("Game Over");
        return;  // Blocca le azioni se la partita è finita
    }

    // Passa il turno all'IA
    CurrentMovementTurn = EMovementTurn::AI;
    UpdateMovementMessage(TEXT("AI Turn: It's his turn to move or attack"));
    MoveAIUnits();
}

// Rappresentazione delle celle tramite lettera-numero
FString AMyGameMode::GetCellIdentifier(AGridCell* Cell)
{
    if (!Cell)
    {
        return FString("Unknown");
    }

    char Letter = 'A' + Cell->GridX;
    int Number = Cell->GridY + 1;
    return FString::Printf(TEXT("%c%d"), Letter, Number);
}
