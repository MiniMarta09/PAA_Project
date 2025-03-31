#include "SniperUnit.h"
#include "UObject/ConstructorHelpers.h"

// Costruttore della classe ASniperUnit: inizializza il mesh, il materiale e le caratteristiche specifiche
ASniperUnit::ASniperUnit()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SniperMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (SniperMesh.Succeeded())
    {
        UnitMesh->SetStaticMesh(SniperMesh.Object);
    }

    // Carica il materiale verde per il giocatore
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> SniperMaterial(TEXT("/Script/Engine.Material'/Game/Materials/M_SniperGreen.M_SniperGreen'"));
    if (SniperMaterial.Succeeded())
    {
        UnitMesh->SetMaterial(0, SniperMaterial.Object);
    }

    // Imposta la scala del mesh
    UnitMesh->SetWorldScale3D(FVector(1.5f, 1.5f, 1.5f));

    MovementRange = 3;
    AttackType = TEXT("Ranged Attack"); 
    AttackRange = 10;
    MinDamage = 4;
    MaxDamage = 8;
    Health = 20;
    HealthMax = 20;
}

// Funzione chiamata all'inizio del gioco per inizializzare impostazioni dello Sniper
void ASniperUnit::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("Sniper BeginPlay - TeamType: %d"), (int32)TeamType);

    // Caricamento del materiale se lo Sniper appartiene alla squadra dell'AI, materiali rossi per l'AI
    if (TeamType == ETeamType::AI && UnitMesh)
    {
        UMaterialInterface* AISniperMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Script/Engine.Material'/Game/Materials/M_SniperRed.M_SniperRed'"));
        if (AISniperMaterial)
        {
            UnitMesh->SetMaterial(0, AISniperMaterial);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AI Sniper Material not found"));
        }
    }
}
