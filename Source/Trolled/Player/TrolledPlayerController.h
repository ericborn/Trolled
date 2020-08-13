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

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
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

	// called when a player is hit
	UFUNCTION(BlueprintImplementableEvent)
	void OnHitPlayer();

	// client respawn
	UFUNCTION(BlueprintCallable)
	void Respawn();

	// server respawn
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRespawn();

public:
	/**Applies recoil to the camera.
	@param RecoilAmount the amount to recoil by. X is the yaw, Y is the pitch
	@param RecoilSpeed the speed to bump the camera up per second from the recoil
	@param RecoilResetSpeed the speed the camera will return to center at per second after the recoil is finished
	@param Shake an optional camera shake to play with the recoil*/
	void ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed, TSubclassOf<class UCameraShake> Shake = nullptr);

	//The amount of recoil to apply. We store this in a variable as we smoothly apply the recoil over several frames
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilBumpAmount;

	//The amount of recoil the gun has had, that we need to reset (After shooting we slowly want the recoil to return to normal.)
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	FVector2D RecoilResetAmount;

	//The speed at which the recoil bumps up per second
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilSpeed;

	//The speed at which the recoil resets per second
	UPROPERTY(EditDefaultsOnly, Category = "Recoil")
	float CurrentRecoilResetSpeed;

	//The last time that we applied recoil
	UPROPERTY(VisibleAnywhere, Category = "Recoil")
	float LastRecoilTime;

	void Turn(float Rate);
	void LookUp(float Rate);

	// allows reload if alive, otherwise respawn
	void StartReload();

};
