// Fill out your copyright notice in the Description page of Project Settings.

#include "LootableActor.h"
#include "Trolled/Components/InteractionComponent.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Trolled/World/ItemSpawn.h"
#include "Trolled/Items/BaseItem.h"
#include "Trolled/MainCharacter.h"

#define LOCTEXT_NAMESPACE "LootableChest"

// Sets default values
ALootableActor::ALootableActor()
{
	// set the static mesh and root components
	LootContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>("LootContainerMesh");
	SetRootComponent(LootContainerMesh);

	// setup the loot interaction function, its text and root component
	LootInteraction = CreateDefaultSubobject<UInteractionComponent>("LootInteraction");
	LootInteraction->InteractableActionText = LOCTEXT("LootActorText", "Loot");
	LootInteraction->InteractableNameText = LOCTEXT("LootActorName", "Chest");
	LootInteraction->SetupAttachment(GetRootComponent());

	// setup the lootables inventory component, capacity
	Inventory = CreateDefaultSubobject<UInventoryComponent>("Inventory");
	Inventory->SetInventoryCapacity(50);
	Inventory->SetWeightCapacity(2000.f);

	// loads between 2 to 8 items into the chest
	LootRolls = FIntPoint(2, 8);

	// replicate the loot to all players
	SetReplicates(true);
}

// Called when the game starts or when spawned
void ALootableActor::BeginPlay()
{
	Super::BeginPlay();

	// gives the actor the on interact function
	LootInteraction->OnInteract.AddDynamic(this, &ALootableActor::OnInteract);

	// if the server with a valid loot table
	if (HasAuthority() && LootTable)
	{
		// get the rows of the loot table
		TArray<FLootTableRow*> SpawnItems;
		LootTable->GetAllRows("", SpawnItems);

		// select a random number between the range defined in the constructor
		int32 Rolls = FMath::RandRange(LootRolls.GetMin(), LootRolls.GetMax());

		// loop that many times
		for (int32 i = 0; i < Rolls; ++i)
		{
			// grab a random row from the loot table
			const FLootTableRow* LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];

			// check that the row is valid
			ensure(LootRow);

			// select a random number between 0 and 1
			float ProbabilityRoll = FMath::FRandRange(0.f, 1.f);

			// if the number is greater than the lootrow probability
			// select a new item and roll again until and item is successfully added
			while (ProbabilityRoll > LootRow->Probability)
			{
				LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];
				ProbabilityRoll = FMath::FRandRange(0.f, 1.f);
			}

			// Not sure what Items is referencing, Items may need to be InventoryArray if its coming from inventory component
			// check for valid lootrow and how many items are in lootrow
			if (LootRow && LootRow->Items.Num())
			{
				// for each of those items
				for (auto& ItemClass : LootRow->Items)
				{
					// if the item has a valid class
					if (ItemClass)
					{
						// get the default quantity of the item
						const int32 Quantity = Cast<UBaseItem>(ItemClass->GetDefaultObject())->GetQuantity();
						
						// try to add the item
						Inventory->TryAddItemFromClass(ItemClass, Quantity);
					}
				}
			}
		}
	}
	
}

void ALootableActor::OnInteract(class AMainCharacter* Character) 
{
	
}

#undef LOCTEXT_NAMESPACE