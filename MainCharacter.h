// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "MainCharacter.generated.h"

// setup properties for character interacting with objects
USTRUCT()
struct FInteractionData
{
	GENERATED_BODY()

	// setup defaults
	FInteractionData()
	{
		ViewedInteractionComponent = nullptr;
		LastInteractionCheckTime = 0.f;
		bInteractHeld = false;
	}

	// stores the interactable component that player is looking at
	UPROPERTY()
	class UInteractionComponent* ViewedInteractionComponent;

	// Time of last interactable check
	// used to avoid having to check every tick for performance reasons
	UPROPERTY()
	float LastInteractionCheckTime;

	// checks if the player is holding the interact button
	UPROPERTY()
	bool bInteractHeld;
};

// delegate for updating UI when an item is equipped, takes slot and item
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquippedItemsChanged, const EEquippableSlot, Slot, const UEquippableItem*, Item);

UCLASS()
class TROLLED_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMainCharacter();

	// The map uses a key which maps to a skeletal mesh component so the equipment goes to the correct slot
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TMap<EEquippableSlot, USkeletalMeshComponent*> PlayerMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UInventoryComponent* PlayerInventory;

	// create camera
	UPROPERTY(EditAnywhere, Category = "Camera")
	class UCameraComponent* CameraComponent;

	// create classes for each of the skeletal mesh areas of the character
	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HelmetMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* ChestMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* LegsMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* FeetMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* VestMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* HandsMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class USkeletalMeshComponent* BackpackMesh;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Tick function, called every frame
	virtual void Tick(float DeltaTime) override;

	// how often in seconds to check for an interactable object. 0 means every tick
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency;

	// how far to trace for an interactable object
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckDistance;

	// checks if theres an interactable item in view
	void PerformInteractionCheck();

	// helper functions for PerformInteractionCheck
	void NoFoundInteractable();
	void FoundNewInteractable(UInteractionComponent* Interactable);

	// called for player pushing interact button
	void BeginInteract();
	void EndInteract();

	// RPC call to server for player pushing interact button
	// reliable forces the client to send up to the server
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerBeginInteract();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEndInteract();

	// called once a player has successfully interacted
	void Interact();

	// stores information about the current state of the players interaction
	UPROPERTY()
	FInteractionData InteractionData;

	// helper to make grabbing the interactable faster
	FORCEINLINE class UInteractionComponent* GetInteractable() const { return InteractionData.ViewedInteractionComponent; }

	// create a timer handle to keep track of players interaction time
	FTimerHandle TimerHandle_Interact;

public:

	// equip and unequip item
	bool EquipItem(class UEquippableItem* Item);
	bool UnEquipItem(class UEquippableItem* Item);

	// These should never be called directly - UGearItem and UWeaponItem call these on top of EquipItem
	void EquipGear(class UGearItem* Gear);
	void UnEquipGear(const EEquippableSlot Slot);

	// called to update the inventory UI when an item is equipped or unequipped
	UPROPERTY(BlueprintAssignable, Category = "Items")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	// returns skeletal mesh component when a lot is passed in
	UFUNCTION(BlueprintPure)
	class USkeletalMeshComponent* GetSlotSkeletalMeshComponent(const EEquippableSlot Slot);


public:
	
	// store if currently interacting
	bool IsInteracting() const;

	// store remaining interaction time
	float GetRemainingInteractTime() const;

	// Items 
	// use item from inventory
	UFUNCTION(BlueprintCallable, Category = "Items")
	void UseItem(class UBaseItem* Item);

	// server use item from inventory
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseItem(class UBaseItem* Item);

	// drop item from inventory
	UFUNCTION(BlueprintCallable, Category = "Items")
	void DropItem(class UBaseItem* Item, const int32 Quantity);

	// server drop item from inventory
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropItem(class UBaseItem* Item, const int32 Quantity);

	// subclass of pickup since it uses bp as base class
	UPROPERTY(EditDefaultsOnly, Category = "Items")
	TSubclassOf<class APickupBase> PickupClass;

protected:
	
	// Creates a map of current equipped items, allows for quick access of equipped items
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TMap<EEquippableSlot, UEquippableItem*> EquippedItems;
	
	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	// called for crouching
	void StartCrouching();
	void StopCrouching();

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

public:	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
