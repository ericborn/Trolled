// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// create and store character camera then attach to mesh
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	CameraComponent->SetupAttachment(GetMesh());
	
	//follows the control rotation which comes from the mouse input
	CameraComponent->bUsePawnControlRotation = true;

	// create skeletal components for each area of the body then attach using getmesh
	HelmetMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HelmetMesh");
	HelmetMesh->SetupAttachment(GetMesh());

	ChestMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ChestMesh");
	ChestMesh->SetupAttachment(GetMesh());

	LegsMesh = CreateDefaultSubobject<USkeletalMeshComponent>("LegsMesh");
	LegsMesh->SetupAttachment(GetMesh());

	FeetMesh = CreateDefaultSubobject<USkeletalMeshComponent>("FeetMesh");
	FeetMesh->SetupAttachment(GetMesh());

	VestMesh = CreateDefaultSubobject<USkeletalMeshComponent>("VestMesh");
	VestMesh->SetupAttachment(GetMesh());

	HandsMesh = CreateDefaultSubobject<USkeletalMeshComponent>("HandsMesh");
	HandsMesh->SetupAttachment(GetMesh());

	BackpackMesh = CreateDefaultSubobject<USkeletalMeshComponent>("BackpackMesh");
	BackpackMesh->SetupAttachment(GetMesh());

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
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

//////////////////////////////////////////////////////////////////////////
// Input

void AMainCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AMainCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AMainCharacter::StopCrouching);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMainCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMainCharacter::LookUpAtRate);
}

void AMainCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMainCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AMainCharacter::StartCrouching() 
{
	Crouch();
}

void AMainCharacter::StopCrouching() 
{
	UnCrouch();
}