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

// exposed version for item instance that just calls TryAddItem_Internal
FItemAddResult UInventoryComponent::TryAddItem(class UBaseItem* Item) 
{
	return TryAddItem_Internal(Item);
}

// exposed version for item class that just calls TryAddItem_Internal
FItemAddResult UInventoryComponent::TryAddItemFromClass(TSubclassOf<class UBaseItem> ItemClass, const int32 Quantity) 
{
	UBaseItem* Item = NewObject<UBaseItem>(GetOwner(), ItemClass);
	Item->SetQuantity(Quantity);
	return TryAddItem_Internal(Item);
}

// remove item from inventory
// TODO: Create function like RemoveItem, but for multiple items
bool UInventoryComponent::RemoveItem(class UBaseItem* Item) 
{
	// Check to force the server to remove the item, not the client. Prevents cheating or improper removal of items client side
	// !!! GetOwner()->HasAuthority() didnt work when previously implemented on character? !!!
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (item)
		{
			// remove item from inventory array
			InventoryArray.RemoveSingle(Item);

			// increments the rep key so the server knows it needs to push updates to the client
			ReplicatedItemsKey++;

			return true;
		}
	}

	// if no item was removed
	return false;
}

// checks if the item has the current quantity or greater
bool UInventoryComponent::HasItemQuantity(TSubclassOf <class UBaseItem> ItemClass, const int32 Quantity) const
{
	if (UBaseItem* ItemToFind = FindItemByClass(ItemClass))
	{
		return ItemToFind->GetQuantity() >= Quantity;
	}
	return false;
}

UBaseItem* UInventoryComponent::FindItem(class UBaseItem* Item) const
{
	// loop through inventory
	for (auto& InvItem : InventoryArray)
	{
		// if item is valid and item has the class we're looking for, return item
		if (InvItem && InvItem->GetClass() == Item->GetClass())
		{
			return InvItem;
		}

	}
	return nullptr;
	
}

// takes in item class, then searches for it in the inventory array
UBaseItem* UInventoryComponent::FindItemByClass(TSubclassOf <class UBaseItem> ItemClass) const
{
	// loop through inventory
	for (auto& InvItem : InventoryArray)
	{
		// if item is valid and item has the class we're looking for, return item
		// ???missing parens on getclass???
		if (InvItem && InvItem->GetClass == ItemClass)
		{
			return InvItem;
		}

	}
	return nullptr;
}

UInventoryComponent::FindAllItemsByClass(TSubclassOf <class UBaseItem> ItemClass) const
{
	// create an array to store all items that match the desired class
	TArray<UBaseItem*> ItemsOfClass;
	
	// loop through inventory
	for (auto& InvItem : InventoryArray)
	{
		// if item is valid and item is a child of the class we're looking for, add to array
		if (InvItem && InvItem->GetClass()->IsChildOf(ItemClass))
		{
			ItemsOfClass.Add(InvItem);
		}

	}
	return ItemsOfClass;
}

float UInventoryComponent::GetCurrentWeight() const
{
	
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity) 
{
	
}

void UInventoryComponent::SetInventoryCapacity(const int32 NewInventoryCapacity) 
{
	
}

// called when items in inventory change
void UInventoryComponent::ClientRefreshInventory_Implementation() 
{
	// calls the delegate to update the UI	
	OnInventoryUpdated.Broadcast();
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

UBaseItem* UInventoryComponent::AddItem(class UBaseItem* Item)
{
	// Check to force the server to add the item, not the client. Prevents cheating or improper added of items client side
	// !!! GetOwner()->HasAuthority() didnt work when previously implemented on character? !!!
	if(GetOwner() && GetOwner()->HasAuthority())
	{
		// recreates a new item from the item being passed in, but sets the owner to the current player and adds to inventory
		UBaseItem* NewItem = NewObject<UBaseItem>(GetOwner(), Item->GetClass());
		NewItem->SetQuantity(Item->GetQuantity());
		NewItem->OwningInventory = this;
		NewItem->AddedToInventory(this);
		InventoryArray.Add(NewItem);
		NewItem->MarkDirtyForReplication();

		return NewItem;
	}

	// if the server isnt the one calling add item, return null
	return nullptr;
}

// called when items in inventory change
void UInventoryComponent::OnRep_Items() 
{
	// calls the delegate to update the UI	
	OnInventoryUpdated.Broadcast();
}

FItemAddResult UInventoryComponent::TryAddItem_Internal(class UBaseItem* Item) 
{
	AddItem(Item);
	return FItemAddResult::AddedAll(Item->Quantity);
}

