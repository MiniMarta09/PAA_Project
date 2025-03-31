#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "GridCell.h"
#include "MyPlayerController.generated.h"

UCLASS()
class PAA_MARTA_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    AMyPlayerController();

private:

    AGridCell* LastHoveredCell;

protected:

    virtual void BeginPlay() override;
    // Configura l'input del mouse
    virtual void SetupInputComponent() override;
    //Proprety per la Camera Actor per impostare la vista dall'alto
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    ACameraActor* MyTopDownCameraActor;

    void ProcessPlayerInput(const float DeltaTime, const bool bGamePaused);
};