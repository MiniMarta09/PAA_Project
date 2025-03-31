#include "GridCell.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "BaseUnit.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameMode.h"

// Costruttore della classe AGridCell: inizializza i componenti base della cella e imposta le proprietà predefinite
AGridCell::AGridCell()
{
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    MeshComponent->SetGenerateOverlapEvents(true);

    // Abilita la collisione per l'attore
    SetActorEnableCollision(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(CubeMesh.Object);
    }

    MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    MeshComponent->bRenderCustomDepth = true;
    MeshComponent->SetCustomDepthStencilValue(1);

    // Inizializza le proprietà della cella
    bIsOccupied = false;
    bIsObstacle = false;
    OccupyingUnit = nullptr;
    GridX = 0;
    GridY = 0;

    InfoBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InfoBox"));
    InfoBox->SetupAttachment(RootComponent);

    InfoBox->SetRelativeLocation(FVector(0, 0, 50));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (BoxMeshAsset.Succeeded())
    {
        InfoBox->SetStaticMesh(BoxMeshAsset.Object);
        InfoBox->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.5f));
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BoxMaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BoxMaterialAsset.Succeeded())
    {
        InfoBox->SetMaterial(0, BoxMaterialAsset.Object);
    }

    InfoBox->SetVisibility(false);
    bIsInfoBoxVisible = false;
}

// Funzione chiamata quando il cursore passa sopra la cella
void AGridCell::OnCellHovered()
{
    InfoBox->SetVisibility(true);
    bIsInfoBoxVisible = true;
}

// Funzione chiamata quando il cursore lascia la cella
void AGridCell::OnCellUnhovered()
{
    InfoBox->SetVisibility(false);
    bIsInfoBoxVisible = false;
}

// Funzione eseguita all'avvio del gioco
void AGridCell::BeginPlay()
{
    Super::BeginPlay();
}

// Funzione per inizializzare la cella con le proprietà:
// bIsAnObstacle: indica se la cella è un ostacolo
// CellSize: rappresenta le dimensioni della cella
// InNormalMaterial: materiale per celle non ostacolo
// InObstacleMaterial: materiale per celle ostacolo (due tipi)
void AGridCell::InitializeCell(bool bIsAnObstacle, const FVector& CellSize, UMaterialInterface* InNormalMaterial, UMaterialInterface* InObstacleMaterial)
{
    NormalMaterial = InNormalMaterial;
    ObstacleMaterial = InObstacleMaterial;  
    bIsObstacle = bIsAnObstacle;

    FVector MeshScale = CellSize / 100.f;
    MeshComponent->SetWorldScale3D(MeshScale);

    if (bIsObstacle)
    {
        if (InObstacleMaterial)
        {
            MeshComponent->SetMaterial(0, InObstacleMaterial);  
        }
    }
    else
    {
        if (InNormalMaterial)
        {
            MeshComponent->SetMaterial(0, InNormalMaterial);  
        }
    }
}

// Imposta lo stato di occupazione della cella
void AGridCell::SetOccupied(bool bOccupied)
{
    bIsOccupied = bOccupied;
}

// Funzione chiamata al click della cella
void AGridCell::OnCellClicked()
{
    if (bIsObstacle || bIsOccupied)
    {
        return;
    }

    AMyGameMode* GameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GameMode)
    {
        GameMode->OnGridCellClicked(this);
    }
}

void AGridCell::HighlightCell(const FLinearColor& Color)
{
    if (!MeshComponent) return;

    UMaterialInstanceDynamic* DynMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
    if (DynMat)
    {
        DynMat->SetVectorParameterValue(TEXT("BaseColor"), Color);
    }
}

void AGridCell::ResetHighlight()
{
    if (MeshComponent)
    {
        if (bIsObstacle && ObstacleMaterial)
        {
            MeshComponent->SetMaterial(0, ObstacleMaterial);
        }
        else if (NormalMaterial)
        {
            MeshComponent->SetMaterial(0, NormalMaterial);
        }
    }
}
