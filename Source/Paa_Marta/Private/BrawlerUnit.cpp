#include "BrawlerUnit.h"
#include "UObject/ConstructorHelpers.h"

// Costruttore della classe ABrawlerUnit: inizializza il mesh, il materiale e le caratteristiche specifiche del Brawler
ABrawlerUnit::ABrawlerUnit()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> BrawlerMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (BrawlerMesh.Succeeded())
    {
        UnitMesh->SetStaticMesh(BrawlerMesh.Object);
    }

    // Carica il materiale verde per il Brawler
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BrawlerMaterial(TEXT("/Script/Engine.Material'/Game/Materials/M_BrawlerGreen.M_BrawlerGreen'"));
    if (BrawlerMaterial.Succeeded())
    {
        UnitMesh->SetMaterial(0, BrawlerMaterial.Object);
    }

    // Imposta la scala del mesh
    UnitMesh->SetWorldScale3D(FVector(1.5f, 1.5f, 1.5f));

    MovementRange = 6;
    AttackType = TEXT("Close-range Attack"); 
    AttackRange = 1;
    MinDamage = 1;
    MaxDamage = 6;
    Health = 40;
    HealthMax = 40;
}

// Funzione chiamata all'inizio del gioco per inizializzare impostazioni del Brawler
void ABrawlerUnit::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("Brawler BeginPlay - TeamType: %d"), (int32)TeamType);

    //Caricamento del materiale se il Brawler appartiene alla squadra dell'AI, materiali rossi per l'AI
    if (TeamType == ETeamType::AI && UnitMesh)
    {
        UMaterialInterface* AIBrawlerMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Script/Engine.Material'/Game/Materials/M_BrawlerRed.M_BrawlerRed'"));
        if (AIBrawlerMaterial)
        {
            UnitMesh->SetMaterial(0, AIBrawlerMaterial);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AI Brawler Material not found"));
        }
    }
}
