#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameHUD.generated.h"

USTRUCT(BlueprintType)
struct FMoveRecord
{
    GENERATED_BODY()

    // Identificativo dell'unità 
    UPROPERTY(BlueprintReadWrite, Category = "Move History")
    FString UnitID;

    // Azione eseguita 
    UPROPERTY(BlueprintReadWrite, Category = "Move History")
    FString Action;

    // Target: per il movimento la cella di destinazione, per l'attacco il danno o la cella nemica
    UPROPERTY(BlueprintReadWrite, Category = "Move History")
    FString Target;

    // Testo formattato che verrà mostrato nello storico delle mosse
    UPROPERTY(BlueprintReadWrite, Category = "Move History")
    FString FormattedText;

    UPROPERTY(BlueprintReadWrite, Category = "Move History")
    float Timestamp;
};

UCLASS()
class PAA_MARTA_API UGameHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    // I widget HUD
    UPROPERTY(EditAnywhere, meta = (BindWidget))
    class UProgressBar* HealthBar;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    class UTextBlock* UnitName_Text;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    class UTextBlock* StartTextFix_Text;

    UPROPERTY(meta = (BindWidget))
    class UScrollBox* ScrollBoxHistoryMoves;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText1;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText2;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText3;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText4;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText5;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText6;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MoveText7;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move History")
    int32 MaxMoveHistorySize = 7;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Move History")
    TArray<FMoveRecord> MoveHistory;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* Start_Text;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* GameOverText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* WinText;

    // Funzioni per l'HUD
    void SetHealthBar(int32 HealthMax, int32 Health);
    void SetUnitName(FText Name);

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetRandomStartText();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void HideStartText();

    // Funzioni per lo storico delle mosse
    UFUNCTION(BlueprintCallable, Category = "Move History")
    void AddMoveToHistory(const FString& UnitID, const FString& Action, const FString& Target);

    UFUNCTION(BlueprintCallable, Category = "Move History")
    void UpdateMoveHistoryDisplay();

    // Questa funzione formatta la stringa e la inserisce nello storico
    UFUNCTION(BlueprintCallable, Category = "Move History")
    void SetExecutionText(const FString& From, const FString& Move, const FString& To);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    bool GetStartWithAI() const { return bAIStartsStart; }

    UFUNCTION(BlueprintCallable, Category = "EndGame")
    void ShowEndGameMessage(bool bPlayerWon);

private:
    bool bAIStartsStart;
};
