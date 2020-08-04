// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

// namespace used for language localization
#define LOCTEXT_NAMESPACE "Inventory"

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

int32 UInventoryComponent::ConsumeAll(Class UBaseItem* Item) 
{
	if (Item)
	{
		ConsumeQuantity(Item, Item->GetQuantity());
	}
	return 0;
}

// only consume if server calls the function
int32 UInventoryComponent::ConsumeQuantity(Class UBaseItem* Item, const int32 Quantity) 
{
	if (GetOwner() && GetOwner()->HasAuthority() && Item)
	{
		// sets RemoveQuantity to the minimum needed to remove desired quantity from the currency quantity
		const int32 RemoveQuantity = FMath::Min(Quantity, Item->GetQuantity());

		// shouldnt be a negative amount of the item after the remove
		ensure(!(Item->GetQuantity() - RemoveQuantity < 0));

		// set the new quantity with the remove applied
		Item->SetQuantity(Item->GetQuantity() - RemoveQuantity);

		// if its 0, remove the item from inventory
		if (Item->GetQuantity() <= 0)
		{
			RemoveItem(Item);
		}
		else
		{
			// forces client to update inventory, reflecting new item quantity
			ClientRefreshInventory();
		}
		
		return RemoveQuantity;
	}
	return 0;
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
		if (InvItem && InvItem->GetClass() == ItemClass)
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

// uses GetStackWeight to multiply current stack size by individual item weight
float UInventoryComponent::GetCurrentWeight() const
{
	float Weight = 0.f;

	for (auto& Item : InventoryArray)
	{
		if(Item)
		{
			Weight += Item->GetStackWeight();
		}
	}

	return Weight;
}

void UInventoryComponent::SetWeightCapacity(const float NewWeightCapacity) 
{
	WeightCapacity = NewWeightCapacity;
	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::SetInventoryCapacity(const int32 NewInventoryCapacity) 
{
	InventoryCapacity = NewInventoryCapacity;
	OnInventoryUpdated.Broadcast();
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
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// determine the quantity the character is trying to add
		const int32 AddAmount = Item->GetQuantity();

		// if current inventory + 1 is bigger than inventory capacity, character is full
		if (InventoryArray.Num() + 1 > GetInventoryCapacity())
		{
			return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryCapacityFullText", "Inventory is full"));
		}

		// This check is not implemented, character will just move slower and slower until they cannot move if too far overweight
		// if item weight is not nearly 0, don't require a weight check
		// if (!FMath::IsNearlyZero(Item->Weight))
		// {
		// 	if (GetCurrentWeight() + Item-Weight > GetWeightCapacity())
		// 	{
		// 		return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryTooMuchWeight", "Carrying too much weight"));
		// 	}
		// }

		// !!! Looks like the video has errors in it. This code stores the item quantity trying to be added in AddAmount,
		// but then uses it as a check for weight, but never calculates a new value. 
		// Weight section isn't being implemented, so probably not a problem
		
		// if item is stackable
		if (Item->bStackable)
		{
			// check that the get quantity is less than or equal to the items max stack size
			ensure(Item->GetQuantity() <= Item->MaxStackSize);

			if (UBaseItem* ExistingItem = FindItem(Item))
			{
				if (ExistingItem->GetQuantity() < ExistingItem->MaxStackSize)
				{
					// find out the max amount that can fit on that stack
					const int32 StackMaxAddAmount = ExistingItem->MaxStackSize - ExistingItem->GetQuantity();

					// takes the smaller of the two, add amount of stack max add amount
					int32 ActualAddAmount = FMath::Min(AddAmount, StackMaxAddAmount);

					FText ErrorText = LOCTEXT("InventoryErrorText", "Couldnt add all of the item to your inventory");
				}

				// // This check is not implemented, character will just move slower and slower until they cannot move if too far overweight
				// if (!FMath::IsNearlyZero(Item->Weight))
				// {
				// 	// find out how much can fit on that stack
				// 	const int32 WeightMaxAddAmount = FMath::FloorToInt((WeightCapacity - GetCurrentWeight()) / Item->Weight);

				// 	// takes the smaller of the two, add amount of stack max add amount
				// 	ActualAddAmount = FMath::Min(AddAmount, WeightMaxAddAmount);

				// 	if (ActualAddAmount < AddAmount)
				// 	{
				// 		ErrorText = Ftext::Format(LOCTEXT("InventoryTooMuchWeight", "Couldn't add entire stack of {ItemName} to inventory"), Item->ItemDisplayName);
				// 	}
				// }
				// // the variables being checked are an exact repeat from above, probably an error in the video
				// else if (ActualAddAmount < AddAmount)
				// {
				// 	ErrorText = Ftext::Format(LOCTEXT("InventoryCapacityFullText", "Couldn't add entire stack of {ItemName} to inventory, inventory is full"), Item->ItemDisplayName);
				// }

				if (ActualAddAmount <= 0)
				{
					return FItemAddResult::AddedNone(AddAmount, LOCTEXT("InventoryErrorText", "Couldnt add any of the item to your inventory");)
				}

				// updates the existing item with its currenty quantity added to the actual add amount
				ExistingItem->SetQuantity(ExistingItem->GetQuantity() + ActualAddAmount);

				// TODO: remove this and create code to make a new stack of the item unless all capacity is taken
				// check that the new quantity isn't greater than the max stack size of the item
				ensure(ExistingItem->GetQuantity() <= ExistingItem->MaxStackSize);
			}
		}

		AddItem(Item);
		return FItemAddResult::AddedAll(Item->Quantity);
	}


}

#undef LOCTEXT_NAMESPACE