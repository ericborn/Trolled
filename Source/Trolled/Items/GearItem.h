// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/EquippableItem.h"
#include "GearItem.generated.h"

/**
 * 
 */

// can create in bp
UCLASS(Blueprintable)
class TROLLED_API UGearItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UGearItem();

	// override Equip/UnEquip to define how the gear items work
	virtual bool Equip(class AMainCharacter* Character) override;
	virtual bool UnEquip(class AMainCharacter* Character) override;

	// skeletal mesh for the gear
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class USkeletalMesh* Mesh;

	// material instance to apply to the gear
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear")
	class UMaterialInstance* MaterialInstance;

	// Damage reduction this item provides, 0.2 = 20% less damage taken. Clamped at 0% and 100%
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Gear", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float DamageReductionMultiplier;
	
};
