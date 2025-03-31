#pragma once

#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "SniperUnit.generated.h"

UCLASS()
class PAA_MARTA_API ASniperUnit : public ABaseUnit
{
    GENERATED_BODY()

public:

    ASniperUnit();

    virtual void BeginPlay() override;
};
