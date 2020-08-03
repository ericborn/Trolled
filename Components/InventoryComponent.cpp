// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Replicates items in inventory to server, so that all other players can access those values.
	// useful for keeping loot on a corpse and preventing players from modding inventory without server knowing
	SetIsReplicated(true);


}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // replicates and makes the server manage the quantity value
    DOREPLIFETIME(UInventoryComponent, InventoryArray);
}

bool UInventoryComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) 
{
	// true when a modification has been made to actor channel
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// check if the array of items needs to replicate
	if (Channel->KeyNeedsToReplicate(0, ReplicatedItemsKey))
	{
		// loops through inventory array
		for (auto& Item : InventoryArray)
		{
			// replicate if item marked as needs to replicate
			if (Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
			{
				bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
			}
		}
	}
	// returns a bool
	return bWroteSomething;
}

void UInventoryComponent::OnRep_Items() 
{
	
}

