// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/DamageType.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InteractionComponent.h"
#include "Trolled/Player/TrolledPlayerController.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Trolled/Weapons/TrolledDamageTypes.h"
#include "Trolled/Weapons/ThrowableWeapon.h"
#include "Trolled/Items/EquippableItem.h"
#include "Trolled/Items/ThrowableItem.h"
#include "Trolled/Items/WeaponItem.h"
#include "Trolled/World/PickupBase.h"
#include "Trolled/Items/GearItem.h"
#include "Trolled/Weapons/Weapon.h"
#include "Trolled/Trolled.h"
#include "Materials/MaterialInstance.h"

#define LOCTEXT_NAMESPACE "MainCharacter"

// define ADS socket
static FName NAME_AimDownSightsSocket("ADSSocket");

// print text
//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Some debug message!"));

// print variable
// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Killed by %s!"), DamageCauser));

/*
!!!server validation!!!
https://docs.unrealengine.com/en-US/Gameplay/Networking/Actors/RPCs/index.html

The tutorials implemented the server functions with validation backwards
Should perform any functionality checks in the bool portion
and the implementation should just handle how it happens

UFUNCTION( Server, WithValidation )
void SomeRPCFunction( int32 AddHealth );

bool SomeRPCFunction_Validate( int32 AddHealth )
{
    if ( AddHealth > MAX_ADD_HEALTH )
    {
        return false;                       // This will disconnect the caller
    }
    return true;                              // This will allow the RPC to be called
}

void SomeRPCFunction_Implementation( int32 AddHealth )
{
    Health += AddHealth;
}
*/

// Seems there are many forms for checking if code is being executed server or client side, not sure on performace differences
// I am using HasAuthority() to indicate server and !HasAuthority() for client
// another form used by the tutorials is GetNetMode() != NM_DedicatedServer or Role < ROLE_Authority, which doesnt work in 4.25
// ROLE_Authority was made private inside aactor and a new method needs to be used for it to work

// Constrcutor of main character, set default values here
AMainCharacter::AMainCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// creates a spring arm, attach to camera socket with a 0 length, length changed on character death
	// repsonsible for keeping the camera from clipping into walls
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>("SpringArmComponent");
	SpringArmComponent->SetupAttachment(GetMesh(), FName("CameraSocket"));
	SpringArmComponent->TargetArmLength = 0.f;

	// create and store character camera then attach to mesh
	CameraComponent = CreateDefaultSubobject<UCameraComponent>("CameraComponent");
	
	// attaches the camera to the spring arm
	CameraComponent->SetupAttachment(SpringArmComponent);
	
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

	// create interaction component, set interaction and name text, bind to root component
	LootPlayerInteraction = CreateDefaultSubobject<UInteractionComponent>("PlayerInteraction");
	LootPlayerInteraction->InteractableActionText = LOCTEXT("LootPlayerText", "Loot");
	LootPlayerInteraction->InteractableNameText = LOCTEXT("LootPlayerName", "Player");
	LootPlayerInteraction->SetupAttachment(GetRootComponent());
	// disable by default
	LootPlayerInteraction->SetActive(false, true);
	LootPlayerInteraction->bAutoActivate = false;

	// check every 0.2, max interaction distance 10m
	InteractionCheckFrequency = 0.2f;
	InteractionCheckDistance = 1000.f;

	// set player stats
	MaxHealth = 100.f;
	Health = MaxHealth;

	MaxStamina = 100.f;
	Stamina = MaxStamina;

	MaxHunger = 100.f;
	Hunger = MaxHunger;

	MaxThirst = 100.f;
	Thirst = MaxThirst;

	// melee attack range and damage
	MeleeAttackDistance = 150.f;
	MeleeAttackDamage = 20.f;

	// default to not ADS
	bIsAiming = false;

	// Allows the character to crouch
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

// Called when the game starts or when spawned
void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	// when a player starts looting, dynamically bind the corpses player name to the corpse
	LootPlayerInteraction->OnInteract.AddDynamic(this, &AMainCharacter::BeginLootingPlayer);

	// player name is stored in the player state, get that name and set the on death loot text to display player name
	if (APlayerState* PS = GetPlayerState())
	{
		LootPlayerInteraction->SetInteractableNameText(FText::FromString(PS->GetPlayerName()));
	}

	// stores values for a character with no armor equipped, so when removing armor later it can return to this
	for (auto& PlayerMesh : PlayerMeshes)
	{
		NakedMeshes.Add(PlayerMesh.Key, PlayerMesh.Value->SkeletalMesh);
	}
	
}

void AMainCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// replicate the loot source to all characters
	DOREPLIFETIME(AMainCharacter, LootSource);

	// replicate current weapon
	DOREPLIFETIME(AMainCharacter, EquippedWeapon);

	// replicate who killed the player
	DOREPLIFETIME(AMainCharacter, Killer);

	// COND_OwnerOnly reps between server and affected client, not all clients.
	// cuts down on traffic being sent between all characters for each others health, stam, etc.
	// if animation changes or mesh effects (blood, broken armour) when a player is hurt
	// DOREPLIFETIME with no CONDITION and COND_OwnerOnly would be necessary
	DOREPLIFETIME_CONDITION(AMainCharacter, Health, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainCharacter, Stamina, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainCharacter, Hunger, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainCharacter, Thirst, COND_OwnerOnly);

	// reps aiming to other players so they see this character perform aim animation
	DOREPLIFETIME_CONDITION(AMainCharacter, bIsAiming, COND_SkipOwner);
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

	// if not the server or a player that is interacting and if the time 
	// since last check is greater than check frequency, check again
	if ((!HasAuthority() || bIsInteractingOnServer) && GetWorld()->TimeSince(InteractionData.LastInteractionCheckTime) > InteractionCheckFrequency)
	{
		// checks if the player is looking at an interactable
		PerformInteractionCheck();
	}

	// only affect aiming for local character
	if (IsLocallyControlled())
	{
		// TODO:
		// need to replace with variables so it can be adjusted by player
		// FOV 70 when ADS, otherwise 100, interp between the two when ADS
		const float DesiredFOV = IsAiming() ? 70.f : 100.f;
		CameraComponent->SetFieldOfView(FMath::FInterpTo(CameraComponent->FieldOfView, DesiredFOV, DeltaTime, 10.f));

		// if you have a weapon
		if (EquippedWeapon)
		{
			// get the ADS socket where the camera should look when ADSing
			const FVector ADSLocation = EquippedWeapon->GetWeaponMesh()->GetSocketLocation(NAME_AimDownSightsSocket);
			
			// get the camera position
			const FVector DefaultCameraLocation = GetMesh()->GetSocketLocation(FName("CameraSocket"));

			// check ADS or not
			const FVector CameraLoc = bIsAiming ? ADSLocation : DefaultCameraLocation;

			// set the interp speed using the ADS time, which can be adjusted per weapon
			const float InterpSpeed = FVector::Dist(ADSLocation, DefaultCameraLocation) / EquippedWeapon->ADSTime;

			// interp the camera between the locations
			CameraComponent->SetWorldLocation(FMath::VInterpTo(CameraComponent->GetComponentLocation(), CameraLoc, DeltaTime, InterpSpeed));
		}
	}
}

void AMainCharacter::Restart() 
{
	Super::Restart();

	// if the controller
	if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(GetController()))
	{
		// from final code
		// // Show gameplay widget again since it was removed when player died
		// if (ASurvivalHUD* HUD = Cast<ASurvivalHUD>(PC->GetHUD()))
		// {
		// 	HUD->CreateGameplayWidget();
		// }
		
		// from video
		PC->ShowIngameUI();
	}
}

float AMainCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) 
//float AMainCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, ATrolledPlayerController* EventInstigator, AActor* DamageCauser) 
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	// mod health with negative damage amount, taking away health
	const float DamageDealt = ModifyHealth(-Damage);

	// if health less than 0
	if (Health <= 0.f)
	{
		// if a player dealt the damage
		if (AMainCharacter* KillerCharacter = Cast<AMainCharacter>(DamageCauser->GetOwner()))
		{
			// killed by player
			KilledByPlayer(DamageEvent, KillerCharacter, DamageCauser);
		}
		else
		{
			// killed by something else
			Killed(DamageEvent, DamageCauser);
		}
	}

	return DamageDealt;
}


void AMainCharacter::SetLootSource(class UInventoryComponent* NewLootSource) 
{
	// if the thing being looting is destroyed, closed the clients loot screen
	if (NewLootSource && NewLootSource->GetOwner())
	{
		NewLootSource->GetOwner()->OnDestroyed.AddUniqueDynamic(this, &AMainCharacter::OnLootSourceOwnerDestroyed);
	}

	// if server
	if (HasAuthority())
	{
		// original designers intent is that corpses only stay in the world for 2 minutes after death
		// if you just start looting the body, it keeps it from despawning for another 2 minutes
		// I dont like this design choice and will find a way to destroy the body but keep loot on the ground
		if (NewLootSource)
		{
			// Looting a player keeps their body alive for an extra 2 minutes to provide enough time to loot their items
			if (AMainCharacter* Character = Cast<AMainCharacter>(NewLootSource->GetOwner()))
			{
				Character->SetLifeSpan(120.f);
			}
		}

		// set loot source to the new player who is looting
		LootSource = NewLootSource;

		// replicate from server down to client to allow them to loot
		OnRep_LootSource();
	}
	else
	{
		// when a player starts looting, ask the server to let the player loot
		ServerSetLootSource(NewLootSource);
	}


}

bool AMainCharacter::IsLooting() const
{
	// if LootSource is not null, we're looting
	return LootSource != nullptr;
}

void AMainCharacter::BeginLootingPlayer(class AMainCharacter* Character) 
{
	// if valid character
	if (Character)
	{
		// set the loot source to the dead players inventory
		Character->SetLootSource(PlayerInventory);
	}
}

void AMainCharacter::OnLootSourceOwnerDestroyed(AActor* DestroyedActor) 
{
	// check for server, valid loot source and an actor that was destroyed was the loot source
	// if the loot source is destroyed, set the source to null at server, which reps back to client
	if (HasAuthority() && LootSource && DestroyedActor == LootSource->GetOwner())
	{
		ServerSetLootSource(nullptr);
	}
}

void AMainCharacter::OnRep_LootSource() 
{
	// if the players controller is calling onrep
	// bring up or remove the looting menu for that player
	if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(GetController()))
	{
		if (PC->IsLocalController())
		{
			// from final, not in video
			// if (ASurvivalHUD* HUD = Cast<ASurvivalHUD>(PC->GetHUD()))
			// {
			// 	if(LootSource)
			// 	{
			// 		HUD->OpenLootWidget();
			// 	}
			// 	else
			// 	{
			// 		HUD->CloseLootWidget();
			// 	}
			// }

			// if the player is the looter
			if(LootSource)
			{
				// show the loot menu
				PC->ShowLootMenu(LootSource);
			}
			else
			{
				// hide the loot menu
				PC->HideLootMenu();
			}
		}
	}
}

void AMainCharacter::LootItem(class UBaseItem* ItemToGive) 
{
	// if server
	if (HasAuthority())
	{
		// valid inventory, loot source, item, item class and quantity being asked for
		if (PlayerInventory && LootSource && ItemToGive && LootSource->HasItemQuantity(ItemToGive->GetClass(), ItemToGive->GetQuantity()))
		{
			// move the item to the requesting player
			const FItemAddResult AddResult = PlayerInventory->TryAddItem(ItemToGive);

			// if that amount is more than 0
			if (AddResult.AmountToGive > 0)
			{
				// remove the item from the loot source
				LootSource->ConsumeQuantity(ItemToGive, AddResult.AmountToGive);
			}
			else
			{
				// check that looting controller is valid
				
				if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(GetController()))
				{
					// tell player why they couldn't loot the item
					PC->ClientShowNotification(AddResult.ErrorText);
				}
			}
		}
	}
	// if player, request server to give loot
	else
	{
		ServerLootItem(ItemToGive);
	}
}

void AMainCharacter::ServerLootItem_Implementation(class UBaseItem* ItemToLoot) 
{
	// call loot item function
	LootItem(ItemToLoot);
}

bool AMainCharacter::ServerLootItem_Validate(class UBaseItem* ItemToLoot) 
{
	return true;
}

void AMainCharacter::ServerSetLootSource_Implementation(class UInventoryComponent* NewLootSource) 
{
	// sets the player as the new loot source when asked to loot
	SetLootSource(NewLootSource);
}

bool AMainCharacter::ServerSetLootSource_Validate(class UInventoryComponent* NewLootSource) 
{
	return true;
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
void AMainCharacter::EquipWeapon(class UWeaponItem* WeaponItem)
{
	// if weapon and class are valid and is server
	if (WeaponItem && WeaponItem->WeaponClass && HasAuthority())
	{
		// if weapon already equipped, unequip first
		if (EquippedWeapon)
		{
			UnEquipWeapon();
		}

		// setup spawn parameters, disable weapons collision, set owner to local player
		FActorSpawnParameters SpawnParams;
		SpawnParams.bNoFail = true;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = SpawnParams.Instigator = this;

		// spawn the weapon in with the class and spawn params
		if (AWeapon* Weapon = GetWorld()->SpawnActor<AWeapon>(WeaponItem->WeaponClass, SpawnParams))
		{
			Weapon->BaseItem = WeaponItem;

			EquippedWeapon = Weapon;
			
			// rep to clients
			OnRep_EquippedWeapon();

			// attach mesh to pawn
			Weapon->OnEquip();
		}
	}
}

// TODO:
// Probably need to modify destroying the weapon to prevent the dumping of all ammo back into inventory 
// and forcing reload on reequip
void AMainCharacter::UnEquipWeapon()
{
	// check server and weapon currently equipped
	if (HasAuthority() && EquippedWeapon)
	{
		EquippedWeapon->OnUnEquip();
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
		OnRep_EquippedWeapon();
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

void AMainCharacter::ServerUseThrowable_Implementation() 
{
	UseThrowable();
}

void AMainCharacter::MulticastPlayThrowableTossFX_Implementation(class UAnimMontage* MontageToPlay) 
{
	//Local player already instantly played grenade throw anim
	// check not the server and not locally controlled, all other players
	if (!HasAuthority() && !IsLocallyControlled())
	{
		// local character plays animation for other local players
		PlayAnimMontage(MontageToPlay);
	}
}

class UThrowableItem* AMainCharacter::GetThrowable() const
{
	// setup an empty var for the throwable
	UThrowableItem* EquippedThrowable = nullptr;

	// if we have a throwable equipped
	if (EquippedItems.Contains(EEquippableSlot::EIS_Throwable))
	{
		// store it in the throwable var
		EquippedThrowable = Cast<UThrowableItem>(*EquippedItems.Find(EEquippableSlot::EIS_Throwable));
	}

	// return new var
	return EquippedThrowable;
}

// Logic from tutorial seems broken and will cause bugs when running out of throwables
void AMainCharacter::UseThrowable()
{
	// check can use
	if (CanUseThrowable())
	{
		// seems uncessessary since this is exactly what the above check does
		if (UThrowableItem* Throwable = GetThrowable())
		{
			// check server side
			if (HasAuthority())
			{
				// spawn projectile
				SpawnThrowable();

				// check valid inventory
				if (PlayerInventory)
				{
					// remove item
					PlayerInventory->ConsumeQuantity(Throwable, 1);
				}
			}
			// may just need to move this else into the HasAuthority above and have it be an else for the if (PlayerInventory)
			// tutorial says without this code when the last throwable is used the UI will not update to reflect
			else
			{
				// throwable 0 or 1
				if (Throwable->GetQuantity() <= 1)
				{
					// remove item from throwable slot, broadcast to others
					EquippedItems.Remove(EEquippableSlot::EIS_Throwable);
					OnEquippedItemsChanged.Broadcast(EEquippableSlot::EIS_Throwable, nullptr);
				}

				//Locally play grenade throw instantly - by the time server spawns the grenade in the throw animation should roughly sync up with the spawning of the grenade
				PlayAnimMontage(Throwable->ThrowableTossAnimation);
				ServerUseThrowable();
			}
		}
	}
}

// Logic I wrote to test a different way to approach the throwable
// void AMainCharacter::UseThrowable() 
// {
// 	// check for valid throwable
// 	if (CanUseThrowable())
// 	{
// 		// if server
// 		if (HasAuthority())
// 		{
// 			// spawn
// 			SpawnThrowable();
			
// 			// rewritten from tutorial because his logic made no sense
// 			// if valid inventory and player has 1 or less, remove from throwable slot
// 			if (PlayerInventory && Throwable->GetQuantity() <= 1)
// 			{
// 				EquippedItems.Remove(EEquippableSlot::EIS_Throwable);
// 				OnEquippedItemsChanged.Broadcast(EEquippableSlot::EIS_Throwable, nullptr);
// 			}
// 			// else player has more than 1
// 			else
// 			{
// 				// remove 1 from players inventory
// 				PlayerInventory->ConsumeQuantity(Throwable, 1);
// 			}
// 		}

// 		//Locally play grenade throw instantly - by the time server spawns the grenade in the throw animation should roughly sync up with the spawning of the grenade
// 		PlayAnimMontage(Throwable->ThrowableTossAnimation);
// 		ServerUseThrowable();
// 		}
// 	}
// }

void AMainCharacter::SpawnThrowable() 
{
	// check is server
	if (HasAuthority())
	{
		// have throwable
		if (UThrowableItem* CurrentThrowable = GetThrowable())
		{
			// seems redundant since thats what get throwable does
			if (CurrentThrowable->ThrowableClass)
			{
				// setup spawn params
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = SpawnParams.Instigator = this;
				SpawnParams.bNoFail = true;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				// setup vars and store for where player is looking
				FVector EyesLoc;
				FRotator EyesRot;
				GetController()->GetPlayerViewPoint(EyesLoc, EyesRot);

				// spawn throwable slightly in front of our face so it doesnt collide with our player
				EyesLoc = (EyesRot.Vector() * 20.f) + EyesLoc;

				// check all pre-reqs are met
				if (AThrowableWeapon* ThrowableWeapon = GetWorld()->SpawnActor<AThrowableWeapon>(CurrentThrowable->ThrowableClass, FTransform(EyesRot, EyesLoc), SpawnParams))
				{
					// tell other local players to play animation
					MulticastPlayThrowableTossFX(CurrentThrowable->ThrowableTossAnimation);
				}
			}
		}
	}
}

bool AMainCharacter::CanUseThrowable() const
{
	// if we have a throwable equipped and it has the throwable class
	return GetThrowable() != nullptr && GetThrowable()->ThrowableClass != nullptr;
}

float AMainCharacter::ModifyHealth(const float Delta) 
{
	// takes current health of player
	const float OldHealth = Health;

	// modify the health
	Health = FMath::Clamp<float>(Health + Delta, 0.f, MaxHealth);

	// return the difference between the two values
	return Health - OldHealth;	
}

void AMainCharacter::OnRep_Health(float OldHealth) 
{
	// rep the modified health value
	OnHealthModified(Health - OldHealth);
}

float AMainCharacter::ModifyStamina(const float Delta) 
{
	// takes current stamina of player
	const float OldStamina = Stamina;

	// modify the stamina
	Stamina = FMath::Clamp<float>(Stamina + Delta, 0.f, MaxStamina);

	// return the difference between the two values
	return Stamina - OldStamina;	
}

void AMainCharacter::OnRep_Stamina(float OldStamina) 
{
	// rep the modified health value
	OnStaminaModified(Stamina - OldStamina);
}

float AMainCharacter::ModifyHunger(const float Delta) 
{
	// takes current stamina of player
	const float OldHunger = Hunger;

	// modify the Hunger
	Hunger = FMath::Clamp<float>(Hunger + Delta, 0.f, MaxHunger);

	// return the difference between the two values
	return Hunger - OldHunger;
}

void AMainCharacter::OnRep_Hunger(float OldHunger) 
{
	// rep the modified health value
	OnHungerModified(Hunger - OldHunger);
}

float AMainCharacter::ModifyThirst(const float Delta) 
{
	// takes current stamina of player
	const float OldThirst = Thirst;

	// modify the Thirst
	Thirst = FMath::Clamp<float>(Thirst + Delta, 0.f, MaxThirst);

	// return the difference between the two values
	return Thirst - OldThirst;
}

void AMainCharacter::OnRep_Thirst(float OldThirst) 
{
	// rep the modified Thirst value
	OnThirstModified(Thirst - OldThirst);
}

void AMainCharacter::OnRep_EquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->OnEquip();
	}
}

void AMainCharacter::StartFire() 
{
	// if weapon equipped, use it, otherwise melee
	if (EquippedWeapon)
	{
		EquippedWeapon->StartFire();
	}
	else
	{
		BeginMeleeAttack();
	}
}

void AMainCharacter::StopFire() 
{
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFire();
	}
}

void AMainCharacter::BeginMeleeAttack() 
{
	// forces the melee swing duration to be the length of the animation
	// preventing swining again until animation is finished
	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength())
	{
		// uses a sphere type collision to check if the attack landed
		// more forgiving than other methods
		FHitResult Hit;
		FCollisionShape Shape = FCollisionShape::MakeSphere(15.f);

		// trace from camera multiplied by the melee attack distance to find the max distance an attack could land
		FVector StartTrace = CameraComponent->GetComponentLocation();
		FVector EndTrace = (CameraComponent->GetComponentRotation().Vector() * MeleeAttackDistance) + StartTrace;

		// setup params for the attack check
		FCollisionQueryParams QueryParams = FCollisionQueryParams("MeleeSweep", false, this);

		// play the melee animation
		PlayAnimMontage(MeleeAttackMontage);

		// check if anything between the player and max melee distance is affectable on the COLLISION_WEAPON channel in the sphere shape
		if (GetWorld()->SweepSingleByChannel(Hit, StartTrace, EndTrace, FQuat(), COLLISION_WEAPON, Shape, QueryParams))
		{
			UE_LOG(LogTemp, Warning, TEXT("HIT!"));
			
			// find the character that was hit
			if (AMainCharacter* HitPlayer = Cast<AMainCharacter>(Hit.GetActor()))
			{
				// get their controller
				if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(GetController()))
				{
					// give the attacking character a hit marker
					// code version
					//PC->ClientShotHitConfirmed();

					// video version
					PC->OnHitPlayer();
				}
			}
		}
		// server process the hit to deal the damage
			ServerProcessMeleeHit(Hit);

		// set time for attack
		LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
	}
}

void AMainCharacter::ServerProcessMeleeHit_Implementation(const FHitResult& MeleeHit) 
{
	// check that time since last melee is greater than melee animation duration, and distance between characters is close enough for damage
	if (GetWorld()->TimeSince(LastMeleeAttackTime) > MeleeAttackMontage->GetPlayLength() && (GetActorLocation() - MeleeHit.ImpactPoint).Size() <= MeleeAttackDistance)
	{
		// tells other players to display own melee animation
		MulticastPlayMeleeFX();
	
		// apply damage to any actor, use BP's to listen for damage
		UGameplayStatics::ApplyPointDamage(MeleeHit.GetActor(), MeleeAttackDamage, (MeleeHit.TraceStart - MeleeHit.TraceEnd).GetSafeNormal(), MeleeHit, GetController(), this, UMeleeDamage::StaticClass());
	}
	// set LastMeleeAttackTime to current time
	LastMeleeAttackTime = GetWorld()->GetTimeSeconds();
}

void AMainCharacter::MulticastPlayMeleeFX_Implementation() 
{
	// if not the local player, play melee local characters attack anim for other characters
	if (!IsLocallyControlled())
	{
		PlayAnimMontage(MeleeAttackMontage);
	}
}

void AMainCharacter::Killed(struct FDamageEvent const& DamageEvent, const AActor* DamageCauser) 
{	
	// if killer is self, set self
	Killer = this;
	OnRep_Killer();
}

void AMainCharacter::KilledByPlayer(struct FDamageEvent const& DamageEvent, class AMainCharacter* Character, const AActor* DamageCauser) 
{
	// code version
	// set killer to character coming into the function
	//Killer = Character;

	// video version
	// cast to player from the event instigators pawn
	// event instigator is the controller that caused the damage
	Killer = Cast<AMainCharacter>(GetInstigator());
	OnRep_Killer();
}

// TODO:
// create inventory drop from corpse after corpse is deleted
// Checks need to be made that if a player can hold 20 items and has 5 items equipped, can they hold 15 more or 20?
// if the equipped items count against total items holdable, when they die with a full inventory and their equipment is unhidden,
// it will over overfill the inventory array and cause an error. May not be an issue if the max equipment cap is removed, 
// or new slots are added as they are unequipped
void AMainCharacter::OnRep_Killer() 
{
	// this controls the length of time a players corpse stays in the world, measured in seconds
	// going to make this longer than suggested until I implement loot droping
	// as a separate item after the corpse is deleted. Currently set to 5 minutes
	SetLifeSpan(300.f);

	// disable collision, enable physics for ragdoll, turn head mesh on for self, disable capsule collision
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GetMesh()->SetOwnerNoSee(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	// wont compile
	//bReplicateMovement = false;
	SetReplicateMovement(false);

	// set loot interaction to active
	LootPlayerInteraction->Activate();

	// check for being the server
	if (HasAuthority())
	{
		// create an array
		TArray<UEquippableItem*> EquippedInvItems;
		// move equipped items into the array
		EquippedItems.GenerateValueArray(EquippedInvItems);

		// iterate through the array and set all the equipped items to false
		// so they appear back in the inventory, they're hidden from inv when equipped
		for (auto& Equippable : EquippedInvItems)
		{
			Equippable->SetEquipped(false);
		}
	}

	// if player
	if (IsLocallyControlled())
	{
		// on death, move camera out 500 units, allow mouse to move camera to look around even while dead
		SpringArmComponent->TargetArmLength = 500.f;
		SpringArmComponent->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		bUseControllerRotationPitch = true;

		// draw deathscreen for local player with killer passed in so it can be drawn on the UI
		if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(GetController()))
		{
			// wont compile until a died is created on controller, but example code has features also unimplemented
			PC->ShowDeathScreen(Killer);
		}
	}
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
		// bp exposed use
		Item->OnUse(this);

		// cpp exposed use
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

	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// was here by default
	//check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &AMainCharacter::StartFire);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &AMainCharacter::StopFire);

	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &AMainCharacter::UseThrowable);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AMainCharacter::StartReload);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &AMainCharacter::StartAiming);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &AMainCharacter::StopAiming);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMainCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AMainCharacter::StopJumping);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMainCharacter::BeginInteract);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AMainCharacter::EndInteract);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AMainCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AMainCharacter::StopCrouching);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMainCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMainCharacter::MoveRight);
}

void AMainCharacter::StartReload() 
{
	// if weapon equipped, reload
	if (EquippedWeapon)
	{
		EquippedWeapon->StartReload();
	}
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

bool AMainCharacter::CanAim() const
{
	// if player has a weapon, allow ADS
	return EquippedWeapon != nullptr;
}

// connects to mouse button to start/stop ADS
void AMainCharacter::StartAiming() 
{
	if (CanAim())
	{
		SetAiming(true);
	}
}

// connects to mouse button to start/stop ADS
void AMainCharacter::StopAiming() 
{
	SetAiming(false);
}

void AMainCharacter::SetAiming(const bool bNewAiming) 
{
	// if trying to aim, but cannot or are already aiming, do nothing
	if ((bNewAiming && !CanAim()) || bNewAiming == bIsAiming)
	{
		return;
	}

	// if client, ask server to aim
	if (!HasAuthority())
	{
		ServerSetAiming(bNewAiming);
	}

	// start aiming
	bIsAiming = bNewAiming;
}

void AMainCharacter::ServerSetAiming_Implementation(const bool bNewAiming) 
{
	// call set aiming
	SetAiming(bNewAiming);
}

#undef LOCTEXT_NAMESPACE