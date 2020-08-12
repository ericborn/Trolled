// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Trolled/Trolled.h"
#include "Trolled/Player/TrolledPlayerController.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Trolled/Items/EquippableItem.h"
#include "Trolled/Items/WeaponItem.h"
#include "Trolled/MainCharacter.h"
#include "Trolled/Items/AmmoItem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
AWeapon::AWeapon()
{
	// setup mesh
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));

	// ignore decals
	WeaponMesh->bReceivesDecals = false;

	// disable weapon collision
	WeaponMesh->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	// set the weapon mesh as the root component
	RootComponent = WeaponMesh;

	// set default state values to false
	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;
	//AttachSocket = FName("GripPoint");
	AttachSocket1P = FName("GripPoint");
	AttachSocket3P = FName("GripPoint");

	CurrentAmmoInMag = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	ADSTime = 0.5f;
	RecoilResetSpeed = 5.f;
	RecoilSpeed = 10.f;

	// setup tick and replication
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;

	// weapon is relevant as long as the current player is relevant
	// cuts down network traffic from updates between players who are too far away to affect each other
	bNetUseOwnerRelevancy = true;
}

// replicate weapon information
void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// rep weapons owner to all
	DOREPLIFETIME(AWeapon, PawnOwner);

	// rep ammo to owner only
	DOREPLIFETIME_CONDITION(AWeapon, CurrentAmmoInMag, COND_OwnerOnly);

	// rep burst and pending reload to all
	DOREPLIFETIME_CONDITION(AWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AWeapon, bPendingReload, COND_SkipOwner);
	
	// COND_InitialOnly only replicates the weapon once to cut net traffic
	DOREPLIFETIME_CONDITION(AWeapon, BaseItem, COND_InitialOnly);
}

// in video, not final code
void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// detach prior to attaching to clear any previous attachments
	//DetachMeshFromPawn();
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// if server, attach main character as pawn owner
	if (HasAuthority())
	{
		PawnOwner = Cast<AMainCharacter>(GetOwner());
	}
}

void AWeapon::Destroyed()
{
	Super::Destroyed();

	// on destroyed, stop fire and fx
	StopSimulatingWeaponFire();
}

// uses ammo from the current mag
void AWeapon::UseMagAmmo()
{
	if (HasAuthority())
	{
		--CurrentAmmoInMag;
	}
}

void AWeapon::ConsumeAmmo(const int32 Amount)
{
	// check for server and valid pawn owner, main character	
	if (HasAuthority() && PawnOwner)
	{
		// get players inventory
		if (UInventoryComponent* Inventory = PawnOwner->PlayerInventory)
		{
			// find the ammo item for the equipped weapon
			if (UBaseItem* AmmoItem = Inventory->FindItemByClass(WeaponConfig.AmmoClass))
			{
				// consume the ammo
				Inventory->ConsumeQuantity(AmmoItem, Amount);
			}
		}
	}
}

// not sure if I understand this properly - video states on unequip of weapon, ammo is returned to inventory
// may need to adjust so that the ammo stays in the weapon itself, so that on unequip/reequip the same number of bullets are in the gun
// otherwise if inventory full and unequip weapon, ammo may fall on the ground or if unequip/reequip the gun may be at full ammo without a reload
void AWeapon::ReturnAmmoToInventory()
{
	//When the weapon is unequipped, try return the players ammo to their inventory
	if (HasAuthority())
	{
		// valid player and ammo greater than 0
		if (PawnOwner && CurrentAmmoInMag > 0)
		{
			// get player inventory
			if (UInventoryComponent* Inventory = PawnOwner->PlayerInventory)
			{
				// add ammo
				Inventory->TryAddItemFromClass(WeaponConfig.AmmoClass, CurrentAmmoInMag);
			}
		}
	}
}

void AWeapon::OnEquip()
{
	// attach weapon to player
	AttachMeshToPawn();

	bPendingEquip = true;

	// state should be equipping
	DetermineWeaponState();

	OnEquipFinished();

	// may need to be changed so that local players can hear equip sound, not just self
	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

// TODO:
// Refactor to disable reloading on equip
void AWeapon::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	// state should be idle
	DetermineWeaponState();

	// automatically tries to reload, seems super annoying and may allow cheating if this bypasses the reload animation
	// need to save the ammo state in the weapon and return that state on equip
	if (PawnOwner)
	{
		// try to reload empty Mag
		if (PawnOwner->IsLocallyControlled() && CanReload())	
		{
			StartReload();
		}
	}
}

// TODO:
// Refactor to disable removing ammo on unequip
void AWeapon::OnUnEquip()
{
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	ReturnAmmoToInventory();
	DetermineWeaponState();
}

bool AWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

void AWeapon::StartFire()
{
	// if not the server, ask server to start firing
	// doesnt compile
	// if (Role < ROLE_Authority)
	if (!HasAuthority())
	{
		// ask the server to start firing
		ServerStartFire();
	}

	// if wants to fire is false
	if (!bWantsToFire)
	{
		// set it to true and check weapon state
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AWeapon::StopFire()
{
	// if not the server, valid pawn and locally controlled
	// (Role < ROLE_Authority) fails to compile, changed to (!HasAuthority())
	//if ((Role < ROLE_Authority) && PawnOwner && PawnOwner->IsLocallyControlled())
	if ((!HasAuthority()) && PawnOwner && PawnOwner->IsLocallyControlled())
	{
		// ask the server to stop firing
		ServerStopFire();
	}

	// if wants to fire is true
	if (bWantsToFire)
	{
		// set to false to stop
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

// bFromReplication checks if the server or player asked to start reloading
void AWeapon::StartReload(bool bFromReplication /*= false*/)
{
	// if local player and not the server
	// (Role < ROLE_Authority) fails to compile, changed to (!HasAuthority())
	// if (!bFromReplication && Role < ROLE_Authority)
	if (!bFromReplication && !HasAuthority())
	{
		// ask server to reload
		ServerStartReload();
	}

	// if the server asked or can reload is true
	if (bFromReplication || CanReload())
	{
		// change state to reloading
		bPendingReload = true;
		DetermineWeaponState();

		// play reload animation
		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = .5f;
		}

		// set timer for reload
		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AWeapon::StopReload, AnimDuration, false);
		if (HasAuthority())
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		// play reload sound for local
		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

// stops reload animation and sets pending reload to false
void AWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}


void AWeapon::ReloadWeapon()
{
	// determine which number is smaller, max ammo per mag minus current ammo in mag, or total current ammo
	// when a players remaining ammo falls below the amount required to fill a mag, the current ammo will be smaller
	// and will then be added to the mag
	const int32 MagDelta = FMath::Min(WeaponConfig.AmmoPerMag - CurrentAmmoInMag, GetCurrentAmmo());

	// if theres more than 0 bullets missing
	if (MagDelta > 0)
	{
		// reload with as many bullets as were missing
		CurrentAmmoInMag += MagDelta;

		// remove ammo from inventory
		ConsumeAmmo(MagDelta);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Didnt have enough ammo for a reload"));
	}
}

bool AWeapon::CanFire() const
{
	// check for valid owner
	bool bCanFire = PawnOwner != nullptr;
	
	// is the weapon idle or already firing
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));

	// return can fire, ok to fire and not reloading
	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false));
}

bool AWeapon::CanReload() const
{
	// check for valid owner
	bool bCanReload = PawnOwner != nullptr;

	// check the current mag has less than max and the player has more than 0 in the inventory
	bool bGotAmmo = (CurrentAmmoInMag < WeaponConfig.AmmoPerMag) && (GetCurrentAmmo() > 0);

	// if weapon idle or firing, set ok to reload
	bool bStateOKToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));

	// return can reload, player has ammo
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true));
}

// check the enum state of the weapon
EWeaponState AWeapon::GetCurrentState() const
{
	return CurrentState;
}

// used for driving UI to track ammo count
int32 AWeapon::GetCurrentAmmo() const
{
	// if the player is valid
	if (PawnOwner)
	{
		// player has a valid inventory
		if (UInventoryComponent* Inventory = PawnOwner->PlayerInventory)
		{
			// look in the inventory for the ammo type desired
			if (UBaseItem* Ammo = Inventory->FindItemByClass(WeaponConfig.AmmoClass))
			{
				// return how much ammo is available
				return Ammo->GetQuantity();
			}
		}
	}

	// else return 0 ammo
	return 0;
}

// find ammo in mag
int32 AWeapon::GetCurrentAmmoInMag() const
{
	return CurrentAmmoInMag;
}

// find max ammo for mag type
int32 AWeapon::GetAmmoPerMag() const
{
	return WeaponConfig.AmmoPerMag;
}

// return the mesh for the weapon
USkeletalMeshComponent* AWeapon::GetWeaponMesh() const
{
	return WeaponMesh;
}

// return the character who owns the weapon
class AMainCharacter* AWeapon::GetPawnOwner() const
{
	return PawnOwner;
}

// set the weapon owner to a main character
void AWeapon::SetPawnOwner(AMainCharacter* MainCharacter)
{
	// if the owner is not the character
	if (PawnOwner != MainCharacter)
	{
		// set the instigator and owner to the character
		SetInstigator(MainCharacter);
		PawnOwner = MainCharacter;
		// net owner for RPC calls
		SetOwner(MainCharacter);
	}
}

// return time the weapon equip started
float AWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

// return weapon equip duration
float AWeapon::GetEquipDuration() const
{
	return EquipDuration;
}

////////////////////////////////
// Server functions

void AWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

void AWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AWeapon::ServerStartFire_Validate()
{
	return true;
}

void AWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool AWeapon::ServerStopFire_Validate()
{
	return true;
}

void AWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool AWeapon::ServerStartReload_Validate()
{
	return true;
}

void AWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

bool AWeapon::ServerStopReload_Validate()
{
	return true;
}

// in final code, not video
// void AWeapon::OnRep_HitNotify()
// {
// 	SimulateInstantHit(HitNotify);
// }

void AWeapon::OnRep_PawnOwner()
{

}

void AWeapon::OnRep_BurstCounter()
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (BurstCounter > 0)
		{
			SimulateWeaponFire();
		}
		else
		{
			StopSimulatingWeaponFire();
		}
	}
}

void AWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload();
	}
	else
	{
		StopReload();
	}
}

void AWeapon::SimulateWeaponFire()
{
	// if server and not currently firing, return
	if (HasAuthority() && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	// particles
	if (MuzzleParticles)
	{
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((PawnOwner != NULL) && (PawnOwner->IsLocallyControlled() == true))
			{
				AController* PlayerCon = PawnOwner->GetController();
				if (PlayerCon != NULL)
				{
					WeaponMesh->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleParticles, WeaponMesh, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleParticles, WeaponMesh, MuzzleAttachPoint);
			}
		}
	}

	// animations
	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		
		// commented out until IsAiming() is implemented
		FWeaponAnim AnimToPlay = FireAnim; //PawnOwner->IsAiming() || PawnOwner->IsLocallyControlled() ? FireAimingAnim : FireAnim;
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	// sound
	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	// recoil, camera shake and gamepad vibration
	ATrolledPlayerController* PC = (PawnOwner != NULL) ? Cast<ATrolledPlayerController>(PawnOwner->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		// apply recoil
		if (RecoilCurve)
		{
			const FVector2D RecoilAmount(RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).X, RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).Y);
			PC->ApplyRecoil(RecoilAmount, RecoilSpeed, RecoilResetSpeed);
		}

		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != NULL)
		{
			FForceFeedbackParameters FFParams;
			FFParams.Tag = "Weapon";
			PC->ClientPlayForceFeedback(FireForceFeedback, FFParams);
		}
	}
}

// deactivate fx, animation, sounds, etc.
void AWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if (MuzzlePSCSecondary != NULL)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAimingAnim);
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}

void AWeapon::HandleHit(const FHitResult& Hit, class AMainCharacter* HitPlayer /*= nullptr*/)
{
	// check for the hit to be on an actor
	if (Hit.GetActor())
	{
		// log the hit
		UE_LOG(LogTemp, Warning, TEXT("Hit actor %s"), *Hit.GetActor()->GetName());
	}

	// pass to the server
	ServerHandleHit(Hit, HitPlayer);

	// check for valid hit player and shooter
	if (HitPlayer && PawnOwner)
	{
		// the pawn owner is a valid controller
		if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(PawnOwner->GetController()))
		{
			// show hit marker on UI
			PC->OnHitPlayer();
		}
	}
}

void AWeapon::ServerHandleHit_Implementation(const FHitResult& Hit, class AMainCharacter* HitPlayer /*= nullptr*/)
{
	if (PawnOwner)
	{
		// set damage multiplier
		float DamageMultiplier = 1.f;

		// Certain bones like head might give extra damage if hit. Apply those.
		for (auto& BoneDamageModifier : HitScanConfig.BoneDamageModifiers)
		{
			if (Hit.BoneName == BoneDamageModifier.Key)
			{
				DamageMultiplier = BoneDamageModifier.Value;
				break;
			}
		}

		if (HitPlayer)
		{
			// apply point damage
			UGameplayStatics::ApplyPointDamage(HitPlayer, HitScanConfig.Damage * DamageMultiplier, (Hit.TraceStart - Hit.TraceEnd).GetSafeNormal(), Hit, PawnOwner->GetController(), this, HitScanConfig.DamageType);
		}
	}
}

// validate the hit
bool AWeapon::ServerHandleHit_Validate(const FHitResult& Hit, class AMainCharacter* HitPlayer /*= nullptr*/)
{
	return true;
}


void AWeapon::FireShot()
{
	if (PawnOwner)
	{
		if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(PawnOwner->GetController()))
		{
			if (RecoilCurve)
			{
				// apply recoil to the controller
				const FVector2D RecoilAmount(RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).X, RecoilCurve->GetVectorValue(FMath::RandRange(0.f, 1.f)).Y);
				PC->ApplyRecoil(RecoilAmount, RecoilSpeed, RecoilResetSpeed, FireCameraShake);
			}

			// get the players aim
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);

			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			
			// ignore self from bullets colliding
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(PawnOwner);

			// commented out until the character has an IsAiming function
			FVector FireDir = CamRot.Vector();// PawnOwner->IsAiming() ? CamRot.Vector() : FMath::VRandCone(CamRot.Vector(), FMath::DegreesToRadians(PawnOwner->IsAiming() ? 0.f : 5.f));
			
			// trace from camera to max fire distance
			FVector TraceStart = CamLoc;
			FVector TraceEnd = (FireDir * HitScanConfig.Distance) + CamLoc;

			// if there was a hit
			if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, COLLISION_WEAPON, QueryParams))
			{
				// store who was hit
				AMainCharacter* HitChar = Cast<AMainCharacter>(Hit.GetActor());

				// hit that play
				HandleHit(Hit, HitChar);

				// draw a debug point to indicate the shot
				FColor PointColor = FColor::Red;
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 5.f, PointColor, false, 30.f);
			}
		}
	}
}

// adjusts subsequent shots for automatic weapons, smoothing out the rapid fire
void AWeapon::HandleReFiring()
{
	UWorld* MyWorld = GetWorld();

	float SlackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - LastFireTime) - WeaponConfig.TimeBetweenShots);

	if (bAllowAutomaticWeaponCatchup)
	{
		TimerIntervalAdjustment -= SlackTimeThisFrame;
	}

	HandleFiring();
}

// client side shooting
void AWeapon::HandleFiring()
{
	// check for ammo and can fire
	if ((CurrentAmmoInMag > 0) && CanFire())
	{
		// if the player, ask server to start shooting
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		// if local, fire, use ammo increment burst counter
		if (PawnOwner && PawnOwner->IsLocallyControlled())
		{
			FireShot();
			UseMagAmmo();

			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	// allow reload
	else if (CanReload())
	{
		StartReload();
	}
	else if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		// if player runs out of ammo
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			// play out of ammo sound fx for local player
			PlayWeaponSound(OutOfAmmoSound);
			ATrolledPlayerController* MyPC = Cast<ATrolledPlayerController>(PawnOwner->Controller);
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{

		// if local player and not the server
		// doesnt work, changed to has authority
		//if (Role < ROLE_Authority)
		if (!HasAuthority())
		{
			ServerHandleFiring();
		}

		// auto reload after firing last round
		if (CurrentAmmoInMag <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleReFiring, FMath::Max<float>(WeaponConfig.TimeBetweenShots + TimerIntervalAdjustment, SMALL_NUMBER), false);
			TimerIntervalAdjustment = 0.f;
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

void AWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;

	// reset firing interval adjustment
	TimerIntervalAdjustment = 0.0f;
}

void AWeapon::SetWeaponState(EWeaponState NewState)
{
	const EWeaponState PrevState = CurrentState;

	// if currently shooting but stop
	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	// if not currently shooting but start
	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

// sets the state of the weapon based upon whats currently happening
void AWeapon::DetermineWeaponState()
{
	EWeaponState NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}

void AWeapon::AttachMeshToPawn()
{
	if (PawnOwner)
	{
		// Remove and hide both first and third person meshes
		//DetachMeshFromPawn();

		if (const USkeletalMeshComponent* PawnMesh = PawnOwner->GetMesh())
		{
			// locally controlled, use first person socket, otherwise use 3rd
			const FName AttachSocket = PawnOwner->IsLocallyControlled() ? AttachSocket1P : AttachSocket3P;
			AttachToComponent(PawnOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocket);
		}
	}
}

// TODO:
// Video states the weapon is destroyed on unequip, needs to be changed so the weapon retains its ammo
// void AWeapon::DetachMeshFromPawn()
// {

// }

// play the weapon sound
UAudioComponent* AWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && PawnOwner)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, PawnOwner->GetRootComponent());
	}

	return AC;
}

// play the weapon animation
float AWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (PawnOwner)
	{
		UAnimMontage* UseAnim = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			Duration = PawnOwner->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}

void AWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (PawnOwner)
	{
		UAnimMontage* UseAnim = PawnOwner->IsLocallyControlled() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			PawnOwner->StopAnimMontage(UseAnim);
		}
	}
}

// probably not used, i think this is handled inside of the fireshot function
// FVector AWeapon::GetCameraAim() const
// {
// 	ATrolledPlayerController* const PlayerController = Instigator ? Cast<ATrolledPlayerController>(Instigator->Controller) : NULL;
// 	FVector FinalAim = FVector::ZeroVector;

// 	if (PlayerController)
// 	{
// 		FVector CamLoc;
// 		FRotator CamRot;
// 		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
// 		FinalAim = CamRot.Vector();
// 	}
// 	else if (Instigator)
// 	{
// 		FinalAim = Instigator->GetBaseAimRotation().Vector();
// 	}

// 	return FinalAim;
// }

// trace for determining hits with weapon
FHitResult AWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, GetInstigator());
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}

// server side for shooting
void AWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInMag > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseMagAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

bool AWeapon::ServerHandleFiring_Validate()
{
	return true;
}