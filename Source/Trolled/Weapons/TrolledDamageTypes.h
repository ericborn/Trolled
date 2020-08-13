// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "TrolledDamageTypes.generated.h"

UCLASS()
class UTrolledDamageBase : public UDamageType
{
	GENERATED_BODY()
};

UCLASS()
class UWeaponDamage : public UTrolledDamageBase
{
	GENERATED_BODY()
};

UCLASS()
class UMeleeDamage : public UTrolledDamageBase
{
	GENERATED_BODY()
};

UCLASS()
class UExplosiveDamage : public UTrolledDamageBase
{
	GENERATED_BODY()
};

UCLASS()
class UVehicleDamage : public UTrolledDamageBase
{
	GENERATED_BODY()
};
