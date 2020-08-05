// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Components/InteractionComponent.h"
#include "Trolled/Items/EquippableItem.h"
#include "Trolled/Items/GearItem.h"
#include "Materials/MaterialInstance.h"
#include "Trolled/World/PickupBase.h"

// Constrcutor of main character, set default values here
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// create and store character camera then attach to mesh
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	
	// attaches the camera to the head socket
	CameraComponent->SetupAttachment(GetMesh(), FName("CameraSocket"));
	
	//follows the control rotation which comes from the mouse input
	CameraComponent->bUsePawnControlRotation = true;

	// adds the EEquippableSlot to the skeletal mesh so items can be attached in each slot
	HelmetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Helmet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HelmetMesh")));
	ChestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Chest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ChestMesh")));
	LegsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Legs, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LegsMesh")));
	FeetMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Feet, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FeetMesh")));
	VestMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Vest, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VestMesh")));
	HandsMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Hands, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandsMesh")));
	BackpackMesh = PlayerMeshes.Add(EEquippableSlot::EIS_Backpack, CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BackpackMesh")));

	// loop through PlayerMeshes which contains all of the slots
	// for each of the slots, use the head mesh attachment and master pose
	for (auto& Kvp : PlayerMeshes)
	{
		USkeletalMeshComponent* MeshComponent = Kvp.Value;
		MeshComponent->SetupAttachment(GetMesh());
		MeshComponent->SetMasterPoseComponent(GetMesh());
	}

	// add head slot last since the head is the root and all other objects need to be attached to it
	PlayerMeshes.Add(EEquippableSlot::EIS_Head, GetMesh());

	// set head to be invisible to self
	GetMesh()->SetOwnerNoSee(true);

	// create inventory component and set default capacities
	PlayerInventory = CreateDefaultSubobject<UInventoryComponent>("PlayerInventory");
	PlayerInventory->SetInventoryCapacity(20);
	PlayerInventory->SetWeightCapacity(60.f);

	// check every 0.2, max interaction distance 10m
	InteractionCheckFrequency = 0.2f;
	InteractionCheckDistance = 1000.f;

	// Hides the head for the player
	GetMesh()->SetOwnerNoSee(true);

	// Allows the character to crouch
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	// stores values for a character with no armor equipped, so when removing armor later it can return to this
	for (auto& PlayerMesh : PlayerMeshes)
	{
		NakedMeshes.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}
	
}

// Called every frame
void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//PerformInteractionCheck();

	// performance checks to prevent the server from checking every tick if a player is looking at an interactable
	// stores true only when a player is interacting
	// !!! ORIGINAL VERSION!!!
	const bool bIsInteractingOnServer = (!HasAuthority() && IsInteracting());

	// !!! FOUND IN SURVIVAL CHAR FILE!!!
	//const bool bIsInteractingOnServer = (GetNetMode() == NM_DedicatedServer && IsInteracting());

	// if not the server or a player that is interacting and if the time 
	// since last check is greater than check frequency, check again
	//if (!HasAuthority() || bIsInteractingOnServer) 
	//&& GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	if ((GetNetMode() != NM_DedicatedServer) && (GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency))
	{
		// checks if the player is looking at an interactable
		PerformInteractionCheck();
		// if (GEngine)
		// {
		// 	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("PerformInteractionCheck!")); 
		// }
	}
	
}

void AMainCharacter::PerformInteractionCheck() 
{
	
	if (GetController() == nullptr)
	{
		return;
	}
	
	// stores the world time
	InteractionData.LastInteractionCheckTime = GetWorld()->GetTimeSeconds();

	// variables for eye location and rotation
	FVector EyesLoc;
	FRotator EyesRot;

	// set those variables from the characters point of view
	GetController()->GetPlayerViewPoint(EyesLoc, EyesRot);

	// Find the max distance the trace can cast from the players current position
	FVector TraceStart = EyesLoc;
	FVector TraceEnd = (EyesRot.Vector() * InteractionCheckDistance) + TraceStart;
	FHitResult TraceHit;

	// prevents own character from being detected by the line trace
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// The line trace returns true if the player is looking at anything but itself
	if (GetWorld()->LineTraceSingleByChannel(TraceHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		// check if the hit was an actor
		if (TraceHit.GetActor())
		{
			// check if it has an interaction component
			if (UInteractionComponent* InteractionComponent = Cast<UInteractionComponent>(TraceHit.GetActor()->GetComponentByClass(UInteractionComponent::StaticClass())))
			{
				// calculates distance to the object
				float Distance = (TraceStart - TraceHit.ImpactPoint).Size();

				// if object is new and distance is less than max interaction distance, found new interactable
				if (InteractionComponent != GetInteractable() && Distance <= InteractionComponent->InteractionDistance)
				{
					FoundNewInteractable(InteractionComponent);
				}
				else if (Distance > InteractionComponent->InteractionDistance && GetInteractable())
				{
					NoFoundInteractable();
				}

				return;
			}
		}
	}

	NoFoundInteractable();
}

void AMainCharacter::NoFoundInteractable() 
{
	// if theres an active timer, but the object is no longer found, clear the timer
	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Interact))
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_Interact);
	}

	// if there was an interactable, stop focus and interact on it
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable-> StopFocus(this);

		if (InteractionData.bInteractHeld)
		{
			EndInteract();
		}
	}

	// clear pointer to the previous interaction component
	InteractionData.ViewedInteractionComponent = nullptr;
}

void AMainCharacter::FoundNewInteractable(UInteractionComponent* Interactable) 
{
	// stops any previous interacts before starting a new one
	EndInteract();

	// stops focus on previous interactable
	if (UInteractionComponent* OldInteractable = GetInteractable())
	{
		OldInteractable-> StopFocus(this);
	}
	
	// grab new Interactable and focus on it
	InteractionData.ViewedInteractionComponent = Interactable;
	Interactable->StartFocus(this);
}

// If not authority(server), call server interact
// while holding interact, bInteractHeld is true
void AMainCharacter::BeginInteract() 
{
	if (!HasAuthority())
	{
		ServerBeginInteract();
	}

	// If an item is non-instant interact, server checks every tick for the duration of the 
	// interact so that it continues to focus as the interaction is happening
	if (HasAuthority())
	{
		PerformInteractionCheck();
	}

	InteractionData.bInteractHeld = true;

	// if item is interactable, start interacting
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->BeginInteract(this);

		// for instant interactions
		if (FMath::IsNearlyZero(Interactable->InteractionTime))
		{
			Interact();
		}
		// sets a timer for interactions that are not instant
		else
		{
			GetWorldTimerManager().SetTimer(TimerHandle_Interact, this, &AMainCharacter::Interact, Interactable->InteractionTime, false);
		}
	}
}

// when interact released, bInteractHeld is false
void AMainCharacter::EndInteract() 
{
		if (!HasAuthority())
	{
		ServerEndInteract();
	}

	// letting go of the interact button sets it back to false
	InteractionData.bInteractHeld = false;

	// clears the timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	// stops the interaction
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->EndInteract(this);
	}
}

// when interact is actually performed
void AMainCharacter::Interact() 
{
	// clear the timer
	GetWorldTimerManager().ClearTimer(TimerHandle_Interact);

	// if the item is interactable, interact with it
	if (UInteractionComponent* Interactable = GetInteractable())
	{
		Interactable->Interact(this);
	}
	
}

bool AMainCharacter::EquipItem(class UEquippableItem* Item)
{
	// adds the item to the map of equipped items, taking in the slot as the key 
	// and item itself as value. Broadcast to the delegate to update UI
	EquippedItems.Add(Item->Slot, Item);
	OnEquippedItemsChanged.Broadcast(Item->Slot, Item);
	return true;
}

bool AMainCharacter::UnEquipItem(class UEquippableItem* Item)
{
	// check item is valid
	if (Item)
	{
		// check slot
		if (EquippedItems.Contains(Item->Slot))
		{
			// check item is equipped
			if (Item == *EquippedItems.Find(Item->Slot))
			{
				// remove from the map, broadcast the change
				EquippedItems.Remove(Item->Slot);
				OnEquippedItemsChanged.Broadcast(Item->Slot, nullptr);
				return true;
			}
		}
	}
	return false;
}

void AMainCharacter::EquipGear(class UGearItem* Gear)
{
	// find slot of gear being equipped and which skeletal mesh comp it attaches to
	if (USkeletalMeshComponent* GearMesh = GetSlotSkeletalMeshComponent(Gear->Slot))
	{
		// set new mesh/texture from gear
		GearMesh->SetSkeletalMesh(Gear->Mesh);
		GearMesh->SetMaterial(GearMesh->GetMaterials().Num() - 1, Gear->MaterialInstance);
	}
}

void AMainCharacter::UnEquipGear(const EEquippableSlot Slot)
{
	// find mesh component to unequip from
	if (USkeletalMeshComponent* EquippableMesh = GetSlotSkeletalMeshComponent(Slot))
	{
		// find naked body mesh stored at begin play
		if (USkeletalMesh* BodyMesh = *NakedMeshes.Find(Slot))
		{
			// set back to naked mesh
			EquippableMesh->SetSkeletalMesh(BodyMesh);

			// reset the materials back on the naked body mesh
			for (int32 i = 0; i < BodyMesh->Materials.Num(); ++i)
			{
				if (BodyMesh->Materials.IsValidIndex(i))
				{
					EquippableMesh->SetMaterial(i, BodyMesh->Materials[i].MaterialInterface);
				}
			}
		}
		else
		{
			//For some gear like backpacks, there is no naked mesh
			EquippableMesh->SetSkeletalMesh(nullptr);
		}
	}
}

class USkeletalMeshComponent* AMainCharacter::GetSlotSkeletalMeshComponent(const EEquippableSlot Slot) 
{
	// if palyer mesh has a slot, return skeletal mesh component
	if (PlayerMeshes.Contains(Slot))
	{
		return *PlayerMeshes.Find(Slot);
	}
	return nullptr;
}

// check if timer is active
bool AMainCharacter::IsInteracting() const
{
	return GetWorldTimerManager().IsTimerActive(TimerHandle_Interact);
}

// returning remaining time
float AMainCharacter::GetRemainingInteractTime() const
{
	return GetWorldTimerManager().GetTimerRemaining(TimerHandle_Interact);
}

void AMainCharacter::UseItem(class UBaseItem* Item) 
{
	// other way to check if you're the client, as the server is ROLE_Authority
	// check for valid item, tell the server to use item to prevent cheating
	
	//if (Role < ROLE_Authority && Item)
	// version above from video throws error: 
	// AActor::Role cannot access private member declared in class 'AActor'
	// Using auth check below which has been used in previous videos
	if(!HasAuthority() && Item)
	{
		ServerUseItem(Item);
	}

	// if server, check for item
	if(HasAuthority())
	{
		//if item not found in inventory, return
		if(PlayerInventory && !PlayerInventory->FindItem(Item))
		{
			return;
		}
	}

	// since use is dependent on item type, call item use function and let it handle 
	// how its used
	if (Item)
	{
		Item->Use(this);
	}
}

void AMainCharacter::ServerUseItem_Implementation(class UBaseItem* Item) 
{
	UseItem(Item);
}

bool AMainCharacter::ServerUseItem_Validate(class UBaseItem* Item) 
{
	return true;
}

void AMainCharacter::DropItem(class UBaseItem* Item, const int32 Quantity) 
{
	// look for valid inventory, item and item in inventory
	if (PlayerInventory && Item && PlayerInventory->FindItem(Item))
	{
		//if (Role < ROLE_Authority && Item)
		// version above from video throws error: 
		// AActor::Role cannot access private member declared in class 'AActor'
		// Using auth check below which has been used in previous videos
		if(!HasAuthority() && Item)
		{
			ServerDropItem(Item, Quantity);
			return;
		}

		// if server, check for item
		if(HasAuthority())
		{
			const int32 ItemQuantity = Item->GetQuantity();
			const int32 DroppedQuantity = PlayerInventory->ConsumeQuantity(Item, Quantity);

			// Spawn params, this player is the owner, cannot fail, try to adjust drop outside of collision meshes (walls, ground, rocks, etc.)
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.bNoFail = true;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			// get players location
			FVector SpawnLocation = GetActorLocation();

			// set location height to half of the capsule
			SpawnLocation.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			// sets rotation of the dropped item to match the players rotation
			FTransform SpawnTransform(GetActorRotation(), SpawnLocation);

			// validation dropped item is a pickup class
			ensure(PickupClass);

			// create the pickup and spawn it in the world using the spawn parameters
			if(APickupBase* Pickup = GetWorld()->SpawnActor<APickupBase>(PickupClass, SpawnTransform, SpawnParams))
			{
				// initalize with class and quantity
				Pickup->InitializePickup(Item->GetClass(), DroppedQuantity);
			}
			
		}
	}
}

void AMainCharacter::ServerDropItem_Implementation(class UBaseItem* Item, const int32 Quantity) 
{
	DropItem(Item, Quantity);
}

bool AMainCharacter::ServerDropItem_Validate(class UBaseItem* Item, const int32 Quantity) 
{
	return true;
}

// start interact on server
void AMainCharacter::ServerBeginInteract_Implementation() 
{
	BeginInteract();
}

// check interact on server
bool AMainCharacter::ServerBeginInteract_Validate() 
{
	return true;
}

// stop interact on server
void AMainCharacter::ServerEndInteract_Implementation() 
{
	EndInteract();
}

// check stopped interact on server
bool AMainCharacter::ServerEndInteract_Validate() 
{
	return true;
}


//////////////////////////////////////////////////////////////////////////
// Input

void AMainCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMainCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AMainCharacter::EndInteract);

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
	if ((Controller != NULL) && (Value != 0.0f))
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