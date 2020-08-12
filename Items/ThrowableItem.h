// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/EquippableItem.h"
#include "ThrowableItem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TROLLED_API UThrowableItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UThrowableItem();

	//The montage to play when we toss a throwable
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	class UAnimMontage* ThrowableTossAnimation;

	//The actor to spawn in when we throw the item. (ie grenade actor, molotov actor, etc)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class AThrowableWeapon> ThrowableClass;	
};
