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

	// Map that stores default mesh to have equipped if we dont have an item equipped - ie the bare skin meshes
	UPROPERTY(BlueprintReadOnly, Category = Mesh)
	TMap<EEquippableSlot, USkeletalMesh*> NakedMeshes;

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

	// array of replicated props
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Tick function, called every frame
	virtual void Tick(float DeltaTime) override;

	// call when a player restarts or respawns, used to remove death screen and reapply main HUD UI
	virtual void Restart() override;

public:

	// function to set what is being looted from
	UFUNCTION(BlueprintCallable)
	void SetLootSource(class UInventoryComponent* NewLootSource);

	// called when a player is looting
	UFUNCTION(BlueprintPure, Category = "Looting")
	bool IsLooting() const;

protected:

	//Begin being looted by a player
	UFUNCTION()
	void BeginLootingPlayer(class AMainCharacter* Character);

	// server validating what is being looted from
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void ServerSetLootSource(class UInventoryComponent* NewLootSource);

	// The inventory that we are currently looting from
	UPROPERTY(ReplicatedUsing = OnRep_LootSource, BlueprintReadOnly)
	UInventoryComponent* LootSource;

	UFUNCTION()
	void OnLootSourceOwnerDestroyed(AActor* DestroyedActor);

	// call when loot source changes, used with BP's for UI actions
	UFUNCTION()
	void OnRep_LootSource();

public:

	// client asking to loot
	UFUNCTION(BlueprintCallable, Category = "Looting")
	void LootItem(class UBaseItem* ItemToGive);

	// server verifying the item is available and can be looted
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerLootItem(class UBaseItem* ItemToLoot);

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

	// returns skeletal mesh component when a slot is passed in
	UFUNCTION(BlueprintPure)
	class USkeletalMeshComponent* GetSlotSkeletalMeshComponent(const EEquippableSlot Slot);

	// helper function that exposes the equipment map to see whats currently equipped
	UFUNCTION(BlueprintPure)
	FORCEINLINE TMap<EEquippableSlot, UEquippableItem*> GetEquippedItems() const { return EquippedItems; }

protected:
	
	// Creates a map of current equipped items, allows for quick access of equipped items
	UPROPERTY(VisibleAnywhere, Category = "Items")
	TMap<EEquippableSlot, UEquippableItem*> EquippedItems;

	// character current health
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health")
	float Health;

	// character max health
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth;

	// character current stamina
	UPROPERTY(ReplicatedUsing = OnRep_Stamina, BlueprintReadOnly, Category = "Stamina")
	float Stamina;

	// character max stamina
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stamina")
	float MaxStamina;

	// character current hunger
	UPROPERTY(ReplicatedUsing = OnRep_Hunger, BlueprintReadOnly, Category = "Hunger")
	float Hunger;

	// character max hunger
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hunger")
	float MaxHunger;

	// character current Thirst
	UPROPERTY(ReplicatedUsing = OnRep_Thirst, BlueprintReadOnly, Category = "Thirst")
	float Thirst;

	// character max Thirst
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thirst")
	float MaxThirst;

public:

	//Modify the players health by either a negative or positive amount. Return the amount of health actually removed
	float ModifyHealth(const float Delta);

	// rep version of health
	UFUNCTION()
	void OnRep_Health(float OldHealth);

	// allows calls to bp for changing screen color when low on health
	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthModified(const float HealthDelta);

	//Modify the players stamina by either a negative or positive amount. Return the amount of stamina actually removed
	float ModifyStamina(const float Delta);

	// rep version of stamina
	UFUNCTION()
	void OnRep_Stamina(float OldStamina);

	// allows calls to bp for changing screen color when low on stamina
	UFUNCTION(BlueprintImplementableEvent)
	void OnStaminaModified(const float StaminaDelta);

	//Modify the players Hunger by either a negative or positive amount. Return the amount of Hunger actually removed
	float ModifyHunger(const float Delta);

	// rep version of hunger
	UFUNCTION()
	void OnRep_Hunger(float OldHunger);

	// allows calls to bp for changing screen color when low on Hunger
	UFUNCTION(BlueprintImplementableEvent)
	void OnHungerModified(const float HungerDelta);

	//Modify the players Thirst by either a negative or positive amount. Return the amount of Thirst actually removed
	float ModifyThirst(const float Delta);

	// rep version of Thirst
	UFUNCTION()
	void OnRep_Thirst(float OldThirst);

	// allows calls to bp for changing screen color when low on Thirst
	UFUNCTION(BlueprintImplementableEvent)
	void OnThirstModified(const float ThirstDelta);

protected:
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
