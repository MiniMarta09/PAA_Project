#include "GameHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

// Imposta la barra della salute in base al rapporto tra vita corrente e vita massima
void UGameHUD::SetHealthBar(int32 HealthMax, int32 Health)
{
    if (HealthBar)
    {
        float Percent = HealthMax > 0 ? static_cast<float>(Health) / HealthMax : 0.0f;
        HealthBar->SetPercent(Percent);

        FLinearColor HealthColor;
        if (Percent > 0.6f)
        {
            HealthColor = FLinearColor::Green;
        }
        else if (Percent > 0.3f)
        {
            HealthColor = FLinearColor::Yellow;
        }
        else
        {
            HealthColor = FLinearColor::Red;
        }

        HealthBar->SetFillColorAndOpacity(HealthColor);
    }
}

// Imposta il nome della unit nella box sopra la barra della salute in base a quale unit ho selezionato
void UGameHUD::SetUnitName(FText Name)
{
    if (UnitName_Text)
    {
        UnitName_Text->SetText(Name);
    }
}

// Funzione di inizializzazione nativa dell'HUD
void UGameHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // Contenuto textblock sopra la barra della salute quando starta il gioco e quindi caso in cui non ho selezionato nessuna unit
    if (UnitName_Text)
    {
        UnitName_Text->SetText(FText::FromString("No unit selected"));
    }

    // Nasconde durante il gioco i messaggi di Game Over e vittoria o sconfitta
    if (GameOverText)
    {
        GameOverText->SetVisibility(ESlateVisibility::Hidden);
    }
    if (WinText)
    {
        WinText->SetVisibility(ESlateVisibility::Hidden);
    }

    SetRandomStartText();
}

// Imposta un messaggio iniziale casuale che indica chi inizia il gioco
void UGameHUD::SetRandomStartText()
{
    if (Start_Text)
    {
        // Genera casualmente true o false e memorizza il risultato in bAIStartsStart
        bAIStartsStart = FMath::RandBool();
        FText NewText = bAIStartsStart ? FText::FromString("AI starts!") : FText::FromString("You start!");
        Start_Text->SetText(NewText);
    }
}

void UGameHUD::HideStartText()
{
    if (Start_Text)
    {
        Start_Text->SetVisibility(ESlateVisibility::Hidden);
    }

    if (StartTextFix_Text)
    {
        StartTextFix_Text->SetVisibility(ESlateVisibility::Hidden);
    }
}

// History Moves gestita nell' HUD da una scrollbar contenente 7 textmove che rappresentano le mosse
// Ogni volta che viene eseguita un'azione attacco o movimento, sia dal giocatore che dall'AI, questa viene aggiunta alla box
// quando viene superato il contenuto massimo della box (7), vengono cancellate le mosse piu vecchie
void UGameHUD::AddMoveToHistory(const FString& UnitID, const FString& Action, const FString& Target)
{
    FMoveRecord NewMove;
    NewMove.UnitID = UnitID;
    NewMove.Action = Action;
    NewMove.Target = Target;

    FString FormattedText;
    if (Action.Equals("Place"))
    {
        FormattedText = UnitID + " place on " + Target;
    }
    else if (Action.Equals("->"))
    {
        FormattedText = UnitID + " --> " + Target;
    }
    else if (Action.Equals("G"))
    {
        FormattedText = UnitID + " " + Target;
    }
    else
    {
        FormattedText = UnitID + " " + Action + " " + Target;
    }
    NewMove.FormattedText = FormattedText;
    NewMove.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    // Inserisce la mossa all'inizio della cronologia
    MoveHistory.Insert(NewMove, 0);

    // Rimuove le mosse in eccesso se la cronologia supera il limite massimo
    if (MoveHistory.Num() > MaxMoveHistorySize)
    {
        MoveHistory.RemoveAt(MaxMoveHistorySize, MoveHistory.Num() - MaxMoveHistorySize);
    }

    UpdateMoveHistoryDisplay();
}

// Registra una mossa con una certa formattazione definita sopra
void UGameHUD::SetExecutionText(const FString& From, const FString& Move, const FString& To)
{
    AddMoveToHistory(From, Move, To);
}

// Aggiorna la visualizzazione della cronologia delle mosse nell'HUD
void UGameHUD::UpdateMoveHistoryDisplay()
{
    TArray<UTextBlock*> MoveTextBlocks;
    MoveTextBlocks.Add(MoveText1);
    MoveTextBlocks.Add(MoveText2);
    MoveTextBlocks.Add(MoveText3);
    MoveTextBlocks.Add(MoveText4);
    MoveTextBlocks.Add(MoveText5);
    MoveTextBlocks.Add(MoveText6);
    MoveTextBlocks.Add(MoveText7);

    // Aggiorna ogni TextBlock con la mossa corrispondente oppure lo nasconde se non ci sono mosse
    for (int32 i = 0; i < MoveTextBlocks.Num(); i++)
    {
        if (MoveTextBlocks[i])
        {
            if (i < MoveHistory.Num())
            {
                MoveTextBlocks[i]->SetVisibility(ESlateVisibility::Visible);
                MoveTextBlocks[i]->SetText(FText::FromString(MoveHistory[i].FormattedText));
            }
            else
            {
                MoveTextBlocks[i]->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }
}

// Mostra il messaggio di fine gioco e il messaggio di vittoria/sconfitta in base al risultato
void UGameHUD::ShowEndGameMessage(bool bPlayerWon)
{
    // Mostra il messaggio "Game Over!"
    if (GameOverText)
    {
        GameOverText->SetText(FText::FromString("Game Over!"));
        GameOverText->SetVisibility(ESlateVisibility::Visible);
    }

    // Imposta il messaggio di vittoria: "You Win!" in verde o "AI Wins!" in rosso
    if (WinText)
    {
        if (bPlayerWon)
        {
            WinText->SetText(FText::FromString("You Win!"));
            WinText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
        }
        else
        {
            WinText->SetText(FText::FromString("AI Wins!")); 
            WinText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
        }
        WinText->SetVisibility(ESlateVisibility::Visible);
    }
}


