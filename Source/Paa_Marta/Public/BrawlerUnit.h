#pragma once

#include "CoreMinimal.h"
#include "BaseUnit.h"
#include "BrawlerUnit.generated.h"

UCLASS()
class PAA_MARTA_API ABrawlerUnit : public ABaseUnit
{
    GENERATED_BODY()

public:

    ABrawlerUnit();

    virtual void BeginPlay() override;
};