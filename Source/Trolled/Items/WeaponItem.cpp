// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponItem.h"
#include "Trolled/MainCharacter.h"
#include "Trolled/Player/TrolledPlayerController.h"

UWeaponItem::UWeaponItem() 
{
    
}

// equip
bool UWeaponItem::Equip(class AMainCharacter* Character) 
{
   	// call super equip from equippable item
    bool bEquipSuccessful = Super::Equip(Character);

	// if equip was succuessful
    if (bEquipSuccessful && Character)
	{
        // spawn the weapon actor
		Character->EquipWeapon(this);
	}

    // return the result
	return bEquipSuccessful; 
}

// unequip
bool UWeaponItem::UnEquip(class AMainCharacter* Character) 
{
    // call super unequip from equippable item
    bool bUnEquipSuccessful = Super::UnEquip(Character);

    // if unequip was succuessful
	if (bUnEquipSuccessful && Character)
	{
		// remove the weapon actor
        Character->UnEquipWeapon();
	}
    
    // return the result
	return bUnEquipSuccessful;
}
