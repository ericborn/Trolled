// Fill out your copyright notice in the Description page of Project Settings.


#include "ThrowableWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AThrowableWeapon::AThrowableWeapon()
{
 	// set static mesh and root component
	ThrowableMesh = CreateDefaultSubobject<UStaticMeshComponent>("ThrowableMesh");
	SetRootComponent(ThrowableMesh);

	// create movement component and speed
	ThrowableMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ThrowableMovement");
	ThrowableMovement->InitialSpeed = 1000.f;

	// rep mesh and movement
	SetReplicates(true);
	SetReplicateMovement(true);
}