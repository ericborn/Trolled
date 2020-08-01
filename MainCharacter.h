// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainCharacter.generated.h"

UCLASS()
class TROLLED_API AMainCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMainCharacter();

	// create camera
	class UCameraComponent* CameraComponent;

	// create classes for each of the skeletal mesh areas of the character
	class USkeletalMeshComponent* HelmetMesh;
	class USkeletalMeshComponent* ChestMesh;
	class USkeletalMeshComponent* LegsMesh;
	class USkeletalMeshComponent* FeetMesh;
	class USkeletalMeshComponent* VestMesh;
	class USkeletalMeshComponent* HandsMesh;
	class USkeletalMeshComponent* BackpackMesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
