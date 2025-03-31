#include "BaseUnit.h"
#include "GridCell.h"
#include "MyGameMode.h"
#include "GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Containers/Queue.h"
#include "Containers/Map.h"

// Restituisce una descrizione dell'unità in base al team e al tipo
FString ABaseUnit::GetUnitDescription(const ABaseUnit* Unit)
{
    if (!Unit)
    {
        return TEXT("Unknown unit"); 
    }
    // HP: human palyer, AI: intelligenza artificiale
    FString TeamLabel = (Unit->TeamType == ETeamType::Player) ? TEXT("HP") : TEXT("AI");

    // Determina il tipo di unità: Sniper o Brawler
    FString UnitTypeLabel;
    if (Unit->UnitType == EUnitType::Sniper)
    {
        UnitTypeLabel = TEXT(" Sniper");
    }
    else if (Unit->UnitType == EUnitType::Brawler)
    {
        UnitTypeLabel = TEXT(" Brawler");
    }
    else
    {
        UnitTypeLabel = TEXT(" Unit");
    }
    return TeamLabel + UnitTypeLabel;
}

// Inizializza i componenti e le proprietà di base delle unit
ABaseUnit::ABaseUnit()
{
    PrimaryActorTick.bCanEverTick = false;
    bHasAttacked = false;
    bHasMoved = false;
    bPendingCounterattack = false;
    PendingCounterDamage = 0;

    // Crea il componente mesh dell'unità
    UnitMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UnitMesh"));
    RootComponent = UnitMesh;

    UnitMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    UnitMesh->SetCollisionResponseToAllChannels(ECR_Block);
    UnitMesh->SetGenerateOverlapEvents(true); 

    UnitMesh->OnClicked.AddDynamic(this, &ABaseUnit::OnUnitClicked);
}

void ABaseUnit::BeginPlay()
{
    Super::BeginPlay();
}

void ABaseUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// Inizializza il tipo e il team delle unit
void ABaseUnit::Initialize(EUnitType Type, ETeamType Team)
{
    UnitType = Type;
    TeamType = Team;
}

// Posiziona l'unità sulla cella della griglia 
void ABaseUnit::PlaceOnGrid(AGridCell* Cell)
{
    if (Cell)
    {
        CurrentCell = Cell;
        FVector CellLocation = Cell->GetActorLocation();
        SetActorLocation(FVector(CellLocation.X, CellLocation.Y, CellLocation.Z + 50.0f));
        Cell->SetOccupied(true);
        Cell->OccupyingUnit = this;
    }
}

// Gestisce il click sull'unità
void ABaseUnit::OnUnitClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
    AMyGameMode* GM = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GM || GM->bGameOver)
    {
        return; // Se il gioco è finito o GameMode non valido, non fare nulla
    }

    if (!CurrentCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("Clicked unit has no valid CurrentCell."));
        return;
    }

    // Aggiorna l'HUD con nome e vita corrente
    if (GM->HUD)
    {
        GM->HUD->SetUnitName(FText::FromString(ABaseUnit::GetUnitDescription(this)));
        GM->HUD->SetHealthBar(HealthMax, Health);
    }

    // Gestione attacco su unità nemiche
    if (TeamType != ETeamType::Player)
    {
        // Controlla che ci sia un'unità selezionata e che questa abbia una cella valida
        if (GM->SelectedUnitForMovement && GM->SelectedUnitForMovement->CurrentCell)
        {
            int32 DeltaX = FMath::Abs(GM->SelectedUnitForMovement->CurrentCell->GridX - CurrentCell->GridX);
            int32 DeltaY = FMath::Abs(GM->SelectedUnitForMovement->CurrentCell->GridY - CurrentCell->GridY);
            int32 ManhattanDistance = DeltaX + DeltaY;

            bool bInRange = false;

            // Verifica il tipo di attacco
            if (GM->SelectedUnitForMovement->AttackType.Equals(TEXT("Ranged Attack")))
            {
                bInRange = (ManhattanDistance <= GM->SelectedUnitForMovement->AttackRange);
            }
            else
            {
                bInRange = (ManhattanDistance == 1);
            }

            if (bInRange)
            {
                GM->SelectedUnitForMovement->AttackTarget(this);

                // Ricontrolla SelectedUnitForMovement dopo l'attacco
                if (GM->SelectedUnitForMovement)
                {
                    GM->SelectedUnitForMovement->bHasAttacked = true;

                    FString TargetCellID = GM->GetCellIdentifier(CurrentCell);
                    int32 Damage = 5;

                    FString UnitPrefix;
                    if (GM->SelectedUnitForMovement->UnitType == EUnitType::Sniper)
                        UnitPrefix = "HP: S";
                    else if (GM->SelectedUnitForMovement->UnitType == EUnitType::Brawler)
                        UnitPrefix = "HP: B";
                    else
                        UnitPrefix = "HP";

                    if (GM->HUD)
                    {
                        GM->HUD->SetExecutionText(UnitPrefix, TargetCellID, FString::FromInt(Damage));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Selected unit was destroyed during attack."));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Enemy out of attack range"));
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Enemy out of attack range"));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No valid unit selected for attack"));
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Select your unit first"));
            }
        }
        return;
    }

    // Gestione clic unit (seleziona/deseleziona)
    if (GM->SelectedUnitForMovement == this)
    {
        GM->ResetAllCellHighlights();
        GM->SelectedUnitForMovement = nullptr;
    }
    else
    {
        GM->DisplayUnitRanges(this);
    }
}


// Muove l'unit verso la nuova cella specificata
void ABaseUnit::MoveToCell(AGridCell* NewCell)
{
    if (!NewCell || NewCell->bIsObstacle || NewCell->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot move: invalid cell"));
        return;
    }

    // Calcola il percorso minimo dalla cella attuale a quella di destinazione
    TArray<AGridCell*> Path = ComputePath(CurrentCell, NewCell);
    if (Path.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No path found to move"));
        return;
    }

    // Libera la cella attuale
    if (CurrentCell)
    {
        CurrentCell->SetOccupied(false);
    }

    // Salva il percorso e inizia il movimento graduale
    MovementPath = Path;
    CurrentPathIndex = 1;
    MoveStep();
}

// Calcola il percorso minimo tra due celle utilizzando la BFS
TArray<AGridCell*> ABaseUnit::ComputePath(AGridCell* Start, AGridCell* Goal)
{
    TArray<AGridCell*> Path;
    if (!Start || !Goal)
    {
        return Path;
    }

    AGridManager* GM = AGridManager::GetInstance(GetWorld());
    if (!GM)
    {
        return Path;
    }

    // Ottiene l'array di tutte le celle dalla GridManager
    const TArray<AGridCell*>& AllCells = GM->GetGridCells();

    // BFS per il percorso minimo
    TQueue<AGridCell*> Frontier;
    TMap<AGridCell*, AGridCell*> CameFrom;
    Frontier.Enqueue(Start);
    CameFrom.Add(Start, nullptr);

    while (!Frontier.IsEmpty())
    {
        AGridCell* Current = nullptr;
        Frontier.Dequeue(Current);

        if (Current == Goal)
        {
            break;
        }

        // Esplora le celle adiacenti 
        for (AGridCell* Cell : AllCells)
        {
            if (!Cell)
            {
                continue;
            }

            int32 ManhattanDistance = FMath::Abs(Cell->GridX - Current->GridX) + FMath::Abs(Cell->GridY - Current->GridY);
            if (ManhattanDistance == 1)
            {
                // Considera la cella se non è un ostacolo e non è occupata 
                if (!Cell->bIsObstacle && (!Cell->bIsOccupied || Cell == Goal))
                {
                    if (!CameFrom.Contains(Cell))
                    {
                        Frontier.Enqueue(Cell);
                        CameFrom.Add(Cell, Current);
                    }
                }
            }
        }
    }

    if (!CameFrom.Contains(Goal))
    {
        return Path;
    }

    AGridCell* Current = Goal;
    while (Current)
    {
        Path.Insert(Current, 0);
        Current = CameFrom.FindRef(Current);
    }
    return Path;
}

// Esegue un passo del movimento lungo il percorso calcolato
void ABaseUnit::MoveStep()
{
    if (CurrentPathIndex < MovementPath.Num())
    {
        AGridCell* NextCell = MovementPath[CurrentPathIndex];
        if (NextCell)
        {
            FVector TargetLocation = NextCell->GetActorLocation() + FVector(0.f, 0.f, 50.f);
            SetActorLocation(TargetLocation);
        }
        CurrentPathIndex++;
        GetWorldTimerManager().SetTimer(MovementTimerHandle, this, &ABaseUnit::MoveStep, MovementStepDelay, false);
    }
    else
    {
        // Movimento completato: aggiorna la cella attuale
        if (MovementPath.Num() > 0)
        {
            AGridCell* FinalCell = MovementPath.Last();
            if (FinalCell)
            {
                FinalCell->SetOccupied(true);
                CurrentCell = FinalCell;
            }
        }
        bHasMoved = true;
        // Se l'unit del giocatore non ha ancora attaccato, visualizza il range aggiornato
        if (TeamType == ETeamType::Player && !bHasAttacked)
        {
            AMyGameMode* GM = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
            if (GM)
            {
                GM->DisplayUnitRanges(this);
            }
        }
    }
}

// Esegue e gestisce l'attacco con tutte le casistiche 
void ABaseUnit::AttackTarget(ABaseUnit* Target)
{
    if (!Target)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttackTarget: Invalid target"));
        return;
    }

    if (bHasAttacked)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s has already attacked this turn"), *GetUnitDescription(this));
        return;
    }

    // Calcola il danno d'attacco (random tra MinDamage e MaxDamage) e lo applico
    int32 Damage = FMath::RandRange(MinDamage, MaxDamage);
    Target->Health -= Damage;

    // Aggiorna l'HUD dell'unit attaccata
    AMyGameMode* GameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode && GameMode->HUD)
    {
        GameMode->HUD->SetHealthBar(Target->HealthMax, Target->Health);
    }

    // Preparazione dei messaggi 
    FString AttackerDesc = GetUnitDescription(this);
    FString TargetDesc = GetUnitDescription(Target);
    FString TurnPrefix = (TeamType == ETeamType::Player) ? TEXT("Player Turn: ") : TEXT("AI Turn: ");
    FString AttackMsg = FString::Printf(TEXT("%s%s attacked %s causing %d damage. %s remaining health: %d"),
        *TurnPrefix, *AttackerDesc, *TargetDesc, Damage, *TargetDesc, Target->Health);
    if (GameMode)
    {
        GameMode->UpdateMovementMessage(AttackMsg);
    }

    // Se l'unit viene eliminata, viene mostrato il messaggio e l'unit viene eliminata
    if (Target->Health <= 0)
    {
        FString DeathMsg = FString::Printf(TEXT("%s has been eliminated!"), *TargetDesc);
        UE_LOG(LogTemp, Warning, TEXT("%s"), *DeathMsg);
        if (GameMode)
        {
            GameMode->UpdateMovementMessage(DeathMsg);
            GameMode->CheckWinCondition();
        }
        Target->Destroy();
    }

    //  Gestione del controattacco:
    // Se l'unit che attacca è uno Sniper e l'unit che sto attaccando è:
    // - uno Sniper
    // - un Brawler a distanza 1
    // allora l'unit che attacca cioè lo Sniper, subisce un danno da controattacco random tra 1 e 3 
    if (UnitType == EUnitType::Sniper)
    {
        bool bCounterattackCondition = false;
        if (Target->UnitType == EUnitType::Sniper)
        {
            bCounterattackCondition = true;
        }
        else if (Target->UnitType == EUnitType::Brawler && CurrentCell && Target->CurrentCell)
        {
            int32 DeltaX = FMath::Abs(CurrentCell->GridX - Target->CurrentCell->GridX);
            int32 DeltaY = FMath::Abs(CurrentCell->GridY - Target->CurrentCell->GridY);
            if ((DeltaX + DeltaY) == 1)
            {
                bCounterattackCondition = true;
            }
        }

        if (bCounterattackCondition)
        {
            // Calcola il danno da controattacco random (tra 1 e 3)
            int32 CounterDamage = FMath::RandRange(1, 3);
            // Applica il danno esclusivamente all'attaccante 
            Health -= CounterDamage;

            FString UnitPrefix = (TeamType == ETeamType::Player) ? TEXT("HP: S") : TEXT("AI: S");
            // Otteniamo la cella in cui si trova l'attaccante
            FString CellID = (GameMode) ? GameMode->GetCellIdentifier(CurrentCell) : TEXT("Unknown");
            FString HistoryMsg = FString::Printf(TEXT("%s %s %d"), *UnitPrefix, *CellID, CounterDamage);

            FString CounterMsg = FString::Printf(TEXT("%s suffered a counterattack causing %d damage. Remaining health: %d"),
                *GetUnitDescription(this), CounterDamage, Health);
            UE_LOG(LogTemp, Warning, TEXT("%s"), *CounterMsg);

            if (GameMode && GameMode->HUD)
            {
                // Aggiorna l'HUD con la nuova salute dell'attaccante
                GameMode->HUD->SetHealthBar(HealthMax, Health);
                // Segnala il controattacco nell'history moves 
                GameMode->HUD->SetExecutionText(HistoryMsg, TEXT("Counterattack"), TEXT(""));

                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, CounterMsg);
            }

            // Se l'attaccante viene eliminato a causa del controattacco, segnala e distruggi l'unità
            if (Health <= 0)
            {
                FString DeathMsg = FString::Printf(TEXT("%s has been eliminated by counterattack!"), *GetUnitDescription(this));
                UE_LOG(LogTemp, Warning, TEXT("%s"), *DeathMsg);
                if (GameMode)
                {
                    GameMode->UpdateMovementMessage(DeathMsg);
                    GameMode->CheckWinCondition();
                }
                Destroy();
            }
        }
    }
    bHasAttacked = true;
}

// Esegue una dummy move e la registra nel log
void ABaseUnit::PerformDummyMove()
{
    bHasMoved = true;
    UE_LOG(LogTemp, Warning, TEXT("%s performs a dummy move."), *GetUnitDescription(this));
}
