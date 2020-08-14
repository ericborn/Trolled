// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TrolledGameMode.generated.h"

UCLASS(minimalapi)
class ATrolledGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATrolledGameMode();


protected:
	//virtual void BeginPlay() override;

	// create an array to hold the AI spawn points
	TArray<class AAISpawnPoint*> AISpawnPoints;

	// timer for spawning in AI
	FTimerHandle TSpawnAIHandle;

	// Spawn AI function
	void SpawnAI();

	// set spawn points for AI
	void SetAISpawnPoints();



};



