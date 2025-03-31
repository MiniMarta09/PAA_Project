#include "GridManager.h"
#include "GridCell.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "EngineUtils.h" 

AGridManager* AGridManager::Instance = nullptr;

// Costruttore della classe AGridManager
AGridManager::AGridManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

// Funzione chiamata all'avvio del gioco: inizializza la griglia (inclusa la generazione degli ostacoli e lo spawn delle celle)
void AGridManager::BeginPlay()
{
    Super::BeginPlay();

    if (!Instance)
    {
        Instance = this;
    }
    else if (Instance != this)
    {
        Destroy();
        return;
    }

    // Inizializza la griglia
    InitializeGrid();
}

// Funzione per generare gli ostacoli sulla griglia e controlla che la griglia rimanga completamente connessa
void AGridManager::GenerateObstacles()
{
    GridObstacles.SetNum(GridRows);
    for (int32 Row = 0; Row < GridRows; Row++)
    {
        GridObstacles[Row].Init(false, GridColumns);
    }

    int32 TotalCells = GridRows * GridColumns;
    int32 MaxObstacles = FMath::RoundToInt(TotalCells * (ObstaclePercentage / 100.0f));
    int32 PlacedObstacles = 0;

    // Crea una lista di tutte le posizioni possibili nella griglia
    TArray<FGridPosition> AllPositions;
    for (int32 Row = 0; Row < GridRows; Row++)
    {
        for (int32 Col = 0; Col < GridColumns; Col++)
        {
            AllPositions.Add(FGridPosition(Col, Row));
        }
    }

    for (int32 i = 0; i < AllPositions.Num(); i++)
    {
        int32 SwapIndex = FMath::RandRange(i, AllPositions.Num() - 1);
        if (i != SwapIndex)
        {
            AllPositions.Swap(i, SwapIndex);
        }
    }

    // Prova ad aggiungere ostacoli, controllando che la griglia rimanga connessa
    for (const FGridPosition& Pos : AllPositions)
    {
        // Imposta temporaneamente la posizione come ostacolo
        GridObstacles[Pos.Y][Pos.X] = true;

        // Verifica se la griglia è ancora completamente connessa
        if (IsGridFullyConnected())
        {
            // Ostacolo accettato
            PlacedObstacles++;
            if (PlacedObstacles >= MaxObstacles)
            {
                break;
            }
        }
        else
        {
            // Ostacolo rifiutato, lo rimuove
            GridObstacles[Pos.Y][Pos.X] = false;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Generated %d obstacles out of a maximum of %d requested."), PlacedObstacles, MaxObstacles);
}

// Con una BFS gestisco la griglia e gli ostacoli in modo da non creare isole (celle in cui non posso arrivare), funzione che chiama a sua volta la funzione FindConnectedCells() per controllare la connettività
bool AGridManager::IsGridFullyConnected()
{
    FGridPosition Start(-1, -1);
    for (int32 Row = 0; Row < GridRows; Row++)
    {
        for (int32 Col = 0; Col < GridColumns; Col++)
        {
            if (!GridObstacles[Row][Col])
            {
                Start = FGridPosition(Col, Row);
                break;
            }
        }
        if (Start.X != -1)
        {
            break;
        }
    }

    // Se non viene trovata alcuna cella libera, la griglia non è valida
    if (Start.X == -1)
    {
        return false;
    }

    // Trova tutte le celle connesse a partire dalla cella trovata
    TSet<FGridPosition> ConnectedCells = FindConnectedCells(Start);

    // Conta il totale delle celle non ostacolo presenti nella griglia
    int32 TotalNonObstacleCells = 0;
    for (int32 Row = 0; Row < GridRows; Row++)
    {
        for (int32 Col = 0; Col < GridColumns; Col++)
        {
            if (!GridObstacles[Row][Col])
            {
                TotalNonObstacleCells++;
            }
        }
    }

    // La griglia è completamente connessa se tutte le celle non ostacolo sono raggiungibili
    return ConnectedCells.Num() == TotalNonObstacleCells;
}

TSet<FGridPosition> AGridManager::FindConnectedCells(const FGridPosition& Start)
{
    const int32 dx[] = { 0, 0, -1, 1 };
    const int32 dy[] = { -1, 1, 0, 0 };

    TSet<FGridPosition> Visited;
    TQueue<FGridPosition> Queue;

    Queue.Enqueue(Start);
    Visited.Add(Start);

    while (!Queue.IsEmpty())
    {
        FGridPosition Current;
        Queue.Dequeue(Current);

        for (int32 Dir = 0; Dir < 4; Dir++)
        {
            int32 NewX = Current.X + dx[Dir];
            int32 NewY = Current.Y + dy[Dir];

            if (NewX >= 0 && NewX < GridColumns && NewY >= 0 && NewY < GridRows)
            {
                FGridPosition NewPos(NewX, NewY);

                if (!GridObstacles[NewY][NewX] && !Visited.Contains(NewPos))
                {
                    Queue.Enqueue(NewPos);
                    Visited.Add(NewPos);
                }
            }
        }
    }

    return Visited;
}

void AGridManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

AGridManager* AGridManager::GetInstance(UWorld* World)
{
    if (!Instance)
    {
        // Cerca un'istanza già esistente nel mondo
        for (TActorIterator<AGridManager> It(World); It; ++It)
        {
            Instance = *It;
            break;
        }
        // Se non viene trovata, crea una nuova istanza
        if (!Instance)
        {
            Instance = World->SpawnActor<AGridManager>(AGridManager::StaticClass(), FTransform::Identity);
        }
    }
    return Instance;
}

// Funzione per inizializzare la griglia: genera ostacoli, calcola offset e crea le celle
void AGridManager::InitializeGrid()
{
    UE_LOG(LogTemp, Warning, TEXT("InitializeGrid() called manually."));

    // Verifica che la classe delle celle sia assegnata
    if (!GridCellClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("GridCellClass has not been assigned in GridManager"));
        return;
    }

    // Verifica che i materiali siano stati assegnati
    if (!NormalMaterial || !ObstacleMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("Materials not assigned in GridManager"));
    }

    // Resetta gli array delle celle e degli ostacoli
    GridCells.Empty();
    GridObstacles.Empty();

    // Genera gli ostacoli e configura l'array GridObstacles
    GenerateObstacles();

    // Calcola la dimensione totale della griglia e l'offset per centrarla
    float TotalWidth = GridColumns * CellSize.X;
    float TotalHeight = GridRows * CellSize.Y;
    FVector OriginOffset = FVector(TotalWidth / 2.f - CellSize.X / 2.f, TotalHeight / 2.f - CellSize.Y / 2.f, 0.f);

    for (int32 Row = 0; Row < GridRows; Row++)
    {
        for (int32 Col = 0; Col < GridColumns; Col++)
        {
            // Definisce il margine tra le celle
            float CellMargin = 10.f;

            // Calcola la posizione della cella includendo il margine e l'offset
            FVector CellLocation = FVector(Col * (CellSize.X + CellMargin), Row * (CellSize.Y + CellMargin), 0.f) - OriginOffset;

            FTransform CellTransform;
            CellTransform.SetLocation(CellLocation);

            bool bIsObstacle = GridObstacles[Row][Col];

            // Seleziona il materiale ostacolo, ci sono due tipi di ostacolo e materiale: alberi e montagne (distribuite in modo casuale)
            UMaterialInterface* FinalObstacleMat = ObstacleMaterial;

            if (bIsObstacle && MountainMaterial && FMath::RandBool())
            {
                FinalObstacleMat = MountainMaterial;
            }

            // Crea una nuova cella
            AGridCell* NewCell = GetWorld()->SpawnActor<AGridCell>(GridCellClass, CellTransform);
            if (NewCell)
            {
                // Inizializza la cella con i materiali e la dimensione specificata
                NewCell->InitializeCell(bIsObstacle, CellSize, NormalMaterial, FinalObstacleMat);

                NewCell->GridX = Col;
                NewCell->GridY = Row;

                GridCells.Add(NewCell);
            }
        }
    }
}

void AGridManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (Instance == this)
    {
        Instance = nullptr;
    }
}
