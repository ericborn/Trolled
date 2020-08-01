// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// create and store character camera then attach to mesh
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(GetMesh());
	
	//follows the control rotation which comes from the mouse input
	CameraComponent->bUsePawnControlRotation = true;

	// create skeletal components for each area of the body
	HelmetMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HelmetMesh");
	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ChestMesh");
	LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>("LegsMesh");
	FeetMesh = CreateDefaultSubobject<USkeletalMeshComponent>("FeetMesh");
	VestMesh = CreateDefaultSubobject<USkeletalMeshComponent>("VestMesh");
	HandsMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HandsMesh");
	BackpackMesh = CreateDefaultSubobject<USkeletalMeshComponent>("BackpackMesh");

}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

