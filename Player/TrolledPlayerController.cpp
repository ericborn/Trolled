// Fill out your copyright notice in the Description page of Project Settings.


#include "TrolledPlayerController.h"
#include "Trolled/MainCharacter.h"
#include "Kismet/GameplayStatics.h"

ATrolledPlayerController::ATrolledPlayerController() 
{
    
}

// void ATrolledPlayerController::Died(class AMainCharacter* Killer) 
// {

// }

// void ATrolledPlayerController::Died(class ASurvivalCharacter* Killer) 
// {
//    	if (ATrolledGameState* GS = Cast<ATrolledGameState>(UGameplayStatics::GetGameState(GetWorld())))
// 	{
// 		//Force the player to respawn 
// 		FTimerHandle DummyHandle;
// 		GetWorldTimerManager().SetTimer(DummyHandle, this, &ATrolledPlayerController::Respawn, GS->RespawnTime, false);

// 		if (ASurvivalHUD* HUD = Cast<ASurvivalHUD>(GetHUD()))
// 		{
// 			HUD->ShowDeathWidget(Killer);
// 		}
// 	} 
// }

void ATrolledPlayerController::ClientShowNotification_Implementation(const FText& Message) 
{
    ShowNotification(Message);
}


