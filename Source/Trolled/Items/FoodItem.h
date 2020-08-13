// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/BaseItem.h"
#include "FoodItem.generated.h"

/**
 * 
 */
UCLASS()
class TROLLED_API UFoodItem : public UBaseItem
{
	GENERATED_BODY()
	
public:

	UFoodItem();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	float HealAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	float StaminaAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	float HungerAmount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	float ThirstAmount;	

// override use so food can be consumed
virtual void Use(class AMainCharacter* Character) override;

};
