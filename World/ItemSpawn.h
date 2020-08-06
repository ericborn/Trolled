// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/TargetPoint.h"
#include "ItemSpawn.generated.h"

// table rows containing the item and its probability of spawning
USTRUCT(BlueprintType)
struct FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	//The item(s) to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<class UBaseItem>> Items;

	//The percentage chance of spawning this item if we hit it on the roll
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = 0.001, ClampMax = 1.0))
	float Probability = 1.f;

};

// from video
//UCLASS(ClassGroup = (Items), Blueprintable, Abstract)
UCLASS()
class TROLLED_API AItemSpawn : public ATargetPoint
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AItemSpawn();

	// data table used to define what can items can be ground spawn
	UPROPERTY(EditAnywhere, Category = "Loot")
	class UDataTable* LootTable;

	// Because pickups use a Blueprint base, we use a UPROPERTY to select it
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<class APickupBase> PickupClass;

	// range of time used for randomly respawning a new pickup 
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	FIntPoint RespawnRange;

	//The item(s) to spawn
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<class UBaseItem>> Items;

	//The percentage chance of spawning this item if we hit it on the roll
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = 0.001, ClampMax = 1.0))
	float Probability = 1.f;

protected:

	// timer handle used for keeping track of time before a respawn
	FTimerHandle TimerHandle_RespawnItem;

	// array of current pickups that exist, used to prevent new items  
	// being spawned in a location that isnt available
	UPROPERTY()
	TArray<AActor*> SpawnedPickups;

	// on BeginPlay spawn the pickups in the world
	virtual void BeginPlay() override;
	
	// spawns the item in the world
	UFUNCTION()
	void SpawnItem();

	// This is bound to the item being destroyed, so we can queue up another item to be spawned in
	UFUNCTION()
	void OnItemTaken(AActor* DestroyedActor);

};

