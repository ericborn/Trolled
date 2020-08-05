// Fill out your copyright notice in the Description page of Project Settings.

#include "EquippableItem.h"
#include "Net/UnrealNetwork.h"
#include "Trolled/MainCharacter.h"
#include "Trolled/Components/InventoryComponent.h"

#define LOCTEXT_NAMESPACE "EquippableItem"

UEquippableItem::UEquippableItem()
{
	bStackable = false;
	bEquipped = false;
	UseActionText = LOCTEXT("EquipText", "Equip");
}

void UEquippableItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquippableItem, bEquipped);
}

// setup use
void UEquippableItem::Use(class AMainCharacter* Character)
{
    // check for character valid and request from server
	if (Character && Character->HasAuthority())
	{
        // if slot already has an item in that slot, remove other item
		if (Character->GetEquippedItems().Contains(Slot) && !bEquipped)
		{
			UEquippableItem* AlreadyEquippedItem = *Character->GetEquippedItems().Find(Slot);

			AlreadyEquippedItem->SetEquipped(false);
		}

        // toggle item trying to be equipped to opposite of current value
		SetEquipped(!IsEquipped());
	}
}

bool UEquippableItem::Equip(class AMainCharacter* Character)
{ 
    // check character is valid, then equip
	if (Character)
	{
		return Character->EquipItem(this);
	}
	return false;
}

bool UEquippableItem::UnEquip(class AMainCharacter* Character)
{
    // check character is valid, then unequip
	if (Character)
	{
		return Character->UnEquipItem(this);
	}
	return false;
}

bool UEquippableItem::ShouldShowInInventory() const 
{
    // when taking off gear, should show back up in inventory
    return !bEquipped;
}

// void UEquippableItem::AddedToInventory(class UInventoryComponent* Inventory)
// {
// 	//If the player looted an item don't equip it
// 	if (AMainCharacter* Character = Cast<AMainCharacter>(Inventory->GetOwner()))
// 	{
// 		if (Character && !Character->IsLooting())
// 		{
// 			/**If we take an equippable, and don't have an item equipped at its slot, then auto equip it*/
// 			if (!Character->GetEquippedItems().Contains(Slot))
// 			{
// 				SetEquipped(true);
// 			}
// 		}
// 	}
// }

void UEquippableItem::SetEquipped(bool bNewEquipped)
{
    // set bEquipped when equipped, update stats then mark for replication
	bEquipped = bNewEquipped;
	EquipStatusChanged();
	MarkDirtyForReplication();
}

void UEquippableItem::EquipStatusChanged()
{
	// find characters outer
    if (AMainCharacter* Character = Cast<AMainCharacter>(GetOuter()))
	{
		// set action text to the appropriate value for gears current status
        UseActionText = bEquipped ? LOCTEXT("UnequipText", "Unequip") : LOCTEXT("EquipText", "Equip");

		// if not equipped than equip, else unequip
        if (bEquipped)
		{
			Equip(Character);
		}
		else
		{
			UnEquip(Character);
		}
	}

	// Tell delegate to update UI
	OnItemModified.Broadcast();
}

#undef LOCTEXT_NAMESPACE