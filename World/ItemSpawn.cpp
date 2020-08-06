// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemSpawn.h"
#include "Trolled/World/PickupBase.h"
#include "Trolled/Items/BaseItem.h"

AItemSpawn::AItemSpawn()
{
	// no tick necessary, item sits stationary until interacted with
    PrimaryActorTick.bCanEverTick = false;
	
    // prevents loading from clients, only server, then replicates them down to clients
    bNetLoadOnClient = false;

    // range of time for the respawn to happen, current 10-30 seconds
	RespawnRange = FIntPoint(10, 30);
}

void AItemSpawn::BeginPlay() 
{
    Super::BeginPlay();

    // if server, spawn item
	if (HasAuthority())
	{
		SpawnItem();
	}
}

void AItemSpawn::SpawnItem() 
{
    // check that we're server and valid loot table
    if (HasAuthority() && LootTable)
	{
        // create an array then load it with loot table items
        TArray<FLootTableRow*> SpawnItems;
		LootTable->GetAllRows("", SpawnItems);

        // select a random loot table roll
		const FLootTableRow* LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];

        // check its valid
		ensure(LootRow);

        // roll for probability
		float ProbabilityRoll = FMath::FRandRange(0.f, 1.f);

        // while loot rows probability is less than the roll
		while (ProbabilityRoll > LootRow->Probability)
		{
            // grab a new item and make a new roll until its not true
			LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];
			ProbabilityRoll = FMath::FRandRange(0.f, 1.f);
		}

        // check the row is valid and has items to spawn of type pickup
        if (LootRow && LootRow->Items.Num() && PickupClass)
		{
            // setup for multi item spawning being in a circular shape
            float Angle = 0.f;

            // loop through the items
            for (auto& ItemClass : LootRow->Items)
			{
				// create an offset from the original spawn location so items dont stack on top of each other
                const FVector LocationOffset = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 50.f;

                // define spawn parameters, prevent fail and attempt to move spawns outside of things with collision (walls, rocks, etc.)
				FActorSpawnParameters SpawnParams;
				SpawnParams.bNoFail = true;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                // spawn with default quantity
				const int32 ItemQuantity = ItemClass->GetDefaultObject<UBaseItem>()->GetQuantity();

                // spawn on the spawn loaction with offset taken into account
				FTransform SpawnTransform = GetActorTransform();
				SpawnTransform.AddToTranslation(LocationOffset);

                // spawn the pickups and bind that item to OnItemTaken, which will trigger the timer to spawn a new item
				APickupBase* Pickup = GetWorld()->SpawnActor<APickupBase>(PickupClass, SpawnTransform, SpawnParams);
				Pickup->InitializePickup(ItemClass, ItemQuantity);
				Pickup->OnDestroyed.AddUniqueDynamic(this, &AItemSpawn::OnItemTaken);

                // add item to array of spawned pickups
				SpawnedPickups.Add(Pickup);

                // create the angle offset for the items based upon total number of spawns, to create circular spawn
				Angle += (PI * 2.f) / LootRow->Items.Num();
			}
        }
    }
}

// TODO:
// May want a system that after a player interacts with any of the spawns it 
// kicks off a timer and eventually deletes and respawns items in this location
void AItemSpawn::OnItemTaken(AActor* DestroyedActor) 
{
    // check that server is taking this acction
    if (HasAuthority())
	{
		// remove the pickup from the array
        SpawnedPickups.Remove(DestroyedActor);

		// if all pickups were taken from this pickup location, queue a respawn
		if (SpawnedPickups.Num() <= 0)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_RespawnItem, this, &AItemSpawn::SpawnItem, FMath::RandRange(RespawnRange.GetMin(), RespawnRange.GetMax()), false);
		}
	}
}