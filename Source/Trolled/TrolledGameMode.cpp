// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrolledGameMode.h"
#include "TrolledCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Trolled/AI/Zombie.h"


ATrolledGameMode::ATrolledGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Character/BP_MainCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

// void ATrolledGameMode::BeginPlay() 
// {
// 	Super::BeginPlay();


	
// }

// void ATrolledGameMode::SpawnAI() 
// {
// 	int RandomIndex = FMath::RandRange(0, )
	
// }
