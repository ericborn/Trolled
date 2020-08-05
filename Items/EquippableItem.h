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

	// slot the item attaches to
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equippables")
	EEquippableSlot Slot;

	// used when the EquipStatusChanged is replicated up to the server
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;

protected:

	// value for the item being equipped or not
	UPROPERTY(ReplicatedUsing = EquipStatusChanged)
	bool bEquipped;

	// called when an item is equipped or unequipped, replicated from client up to server, 
	// then server down to clients so all clients can see each others clothing
	UFUNCTION()
	void EquipStatusChanged();
};
