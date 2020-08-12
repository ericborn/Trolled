// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrowableWeapon.generated.h"

UCLASS()
class TROLLED_API AThrowableWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AThrowableWeapon();

protected:
	
	// static mesh
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UStaticMeshComponent* ThrowableMesh;

	// projectile movement component
	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UProjectileMovementComponent* ThrowableMovement;
};
