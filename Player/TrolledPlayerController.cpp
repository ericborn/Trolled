// Fill out your copyright notice in the Description page of Project Settings.


#include "TrolledPlayerController.h"
#include "Trolled/MainCharacter.h"
#include "Kismet/GameplayStatics.h"

ATrolledPlayerController::ATrolledPlayerController() 
{
    
}

void ATrolledPlayerController::ClientShowNotification_Implementation(const FText& Message) 
{
    ShowNotification(Message);
}


