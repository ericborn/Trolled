// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/EquippableItem.h"
#include "WeaponItem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TROLLED_API UWeaponItem : public UEquippableItem
{
	GENERATED_BODY()
public:
	UWeaponItem();

	// equip and unequip weapons
	virtual bool Equip(class AMainCharacter* Character) override;
	virtual bool UnEquip(class AMainCharacter* Character) override;

	// The weapon class to give to the player upon equipping this weapon item
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AWeapon> WeaponClass;
};
