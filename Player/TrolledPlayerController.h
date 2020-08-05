// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TrolledPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class TROLLED_API ATrolledPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ATrolledPlayerController();

	// pushes a notification from the server down to the client
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void ClientShowNotification(const FText& Message);
	
	// Blueprint Implementable allows functions to be constructed here
	// but implemented in BP's so the two can both control the functions
	UFUNCTION(BlueprintImplementableEvent)
	void ShowNotification(const FText& Message);

	// show the death screen with the player that killed this player
	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class AMainCharacter* Killer);

	// show the loot menu UI
	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootMenu(const class UInventoryComponent* LootSource);

	// show the main game UI
	UFUNCTION(BlueprintImplementableEvent)
	void ShowIngameUI();

	// hide the loot menu UI
	UFUNCTION(BlueprintImplementableEvent)
	void HideLootMenu();

	//
	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

};
