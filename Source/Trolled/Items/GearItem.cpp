// Fill out your copyright notice in the Description page of Project Settings.


#include "GearItem.h"
#include "Trolled/MainCharacter.h"

UGearItem::UGearItem() 
{
    // default is 5% damage reduction
    DamageReductionMultiplier = 0.05f;
}

bool UGearItem::Equip(class AMainCharacter* Character)
{
	// tell the parent to equip
    bool bEquipSuccessful = Super::Equip(Character);

    // if successful, equip this
	if (bEquipSuccessful && Character)
	{
		Character->EquipGear(this);
	}

	return bEquipSuccessful;
}

bool UGearItem::UnEquip(class AMainCharacter* Character)
{
	// tell the parent to unequip
    bool bUnEquipSuccessful = Super::UnEquip(Character);

    // if successful, unequip slot
	if (bUnEquipSuccessful && Character)
	{
		Character->UnEquipGear(Slot);
	}

	return bUnEquipSuccessful;
}
