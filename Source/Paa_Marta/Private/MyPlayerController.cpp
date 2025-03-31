#include "MyPlayerController.h"
#include "MyGameMode.h"
#include "GridCell.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

// Costruttore della classe AMyPlayerController: imposta le proprietà per mostrare il cursore e gestire gli eventi di click
AMyPlayerController::AMyPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

// Funzione chiamata all'avvio del gioco per inizializzare il controller e la telecamera
void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Cerca o crea una CameraActor per la visuale 
    if (!MyTopDownCameraActor)
    {
        MyTopDownCameraActor = Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass()));
        if (!MyTopDownCameraActor)
        {
            FActorSpawnParameters SpawnParams;
            MyTopDownCameraActor = GetWorld()->SpawnActor<ACameraActor>(
                ACameraActor::StaticClass(),
                FVector(0, 0, 1000),   
                FRotator(-90, 0, 0),    
                SpawnParams
            );
        }
    }
    if (MyTopDownCameraActor)
    {
        SetViewTarget(MyTopDownCameraActor);

        UE_LOG(LogTemp, Warning, TEXT("Camera successfully configured"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Error creating camera"));
    }

    // Configura le impostazioni di input 
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);

    // Inizializza la variabile per l'hover delle celle della griglia
    LastHoveredCell = nullptr;
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
}

// Funzione per settare l'input del giocatore ad ogni frame
void AMyPlayerController::ProcessPlayerInput(const float DeltaTime, const bool bGamePaused)
{
    Super::ProcessPlayerInput(DeltaTime, bGamePaused);

    // Gestisce la barra spaziatrice, che premuta ha la funzione di terminare il turno del giocatore
    if (WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        AMyGameMode* GM = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
        if (GM && GM->CurrentMovementTurn == EMovementTurn::Player)
        {
            GM->EndPlayerTurn();
        }
    }

    AMyGameMode* GM = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

    // Gestione del click del mouse
    if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        FHitResult Hit;
        if (GetHitResultUnderCursor(ECC_Visibility, true, Hit) && Hit.GetActor())
        {
            AGridCell* Cell = Cast<AGridCell>(Hit.GetActor());
            if (Cell && GM)
            {
                // Inoltra l'evento di click al GameMode
                GM->OnGridCellClicked(Cell);
            }
        }
    }
}
