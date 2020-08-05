// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/BaseItem.h"
#include "EquippableItem.generated.h"

// slots gear can be equipped to
UENUM(BlueprintType)
enum class EEquippableSlot : uint8
{
	EIS_Head UMETA(DisplayName = "Head"),
	EIS_Helmet UMETA(DisplayName = "Helmet"),
	EIS_Chest UMETA(DisplayName = "Chest"),
	EIS_Vest UMETA(DisplayName = "Vest"),
	EIS_Legs UMETA(DisplayName = "Legs"),
	EIS_Feet UMETA(DisplayName = "Feet"),
	EIS_Hands UMETA(DisplayName = "Hands"),
	EIS_Backpack UMETA(DisplayName = "Backpack"),
	EIS_PrimaryWeapon UMETA(DisplayName = "Primary Weapon"),
	EIS_Throwable UMETA(DisplayName = "Throwable Item")
};

// Base equippable item - Only children can be equipped
UCLASS(Abstract, NotBlueprintable)
class TROLLED_API UEquippableItem : public UBaseItem
{
	GENERATED_BODY()

public:

	UEquippableItem();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippables")
	EEquippableSlot Slot;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;

	// override for how equipment is used
	virtual void Use(class ASurvivalCharacter* Character) override;

	// equip/unequip equipment
	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool Equip(class ASurvivalCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Equippables")
	virtual bool UnEquip(class ASurvivalCharacter* Character);

	// override show/hide and added to inventory for equipment
	virtual bool ShouldShowInInventory() const override;
	virtual void AddedToInventory(class UInventoryComponent* Inventory) override;

	// track if something is equipped
	UFUNCTION(BlueprintPure, Category = "Equippables")
	bool IsEquipped() { return bEquipped; };

	// Call this on the server to equip the item
	void SetEquipped(bool bNewEquipped);

protected:

	UPROPERTY(ReplicatedUsing = EquipStatusChanged)
	bool bEquipped;

	UFUNCTION()
	void EquipStatusChanged();

};
