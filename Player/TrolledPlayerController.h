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
	
	// server side call for notifications 
	UFUNCTION(BlueprintImplementableEvent)
	void ShowNotification(const FText& Message);

	// 
	UFUNCTION(BlueprintImplementableEvent)
	void ShowDeathScreen(class AMainCharacter* Killer);

	// 
	UFUNCTION(BlueprintImplementableEvent)
	void ShowLootMenu(const class UInventoryComponent* LootSource);

	//
	UFUNCTION(BlueprintImplementableEvent)
	void ShowIngameUI();

	//
	UFUNCTION(BlueprintImplementableEvent)
	void HideLootMenu();

	//
	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

};
