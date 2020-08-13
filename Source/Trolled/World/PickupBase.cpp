// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupBase.h"
#include "Trolled/Items/BaseItem.h"
#include "Net/UnrealNetwork.h"
#include "Trolled/MainCharacter.h"
#include "Trolled/Player/TrolledPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Trolled/Components/InteractionComponent.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Engine/ActorChannel.h"

// Sets default values
APickupBase::APickupBase()
{
	// setup mesh
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");

	// no collision between pickups and pawns
	PickupMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// set the PickupMesh as the root component
	SetRootComponent(PickupMesh);

	// add interaction component to the pickup item base
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>("PickupInteractionComponent");

	// set default interaction time, distance, pickup, action text, functionality for when the pickup is taken, bind interaction to the static mesh
	InteractionComponent->InteractionTime = 0.1f;
	InteractionComponent->InteractionDistance = 200.f;
	InteractionComponent->InteractableNameText = FText::FromString("Pickup");
	InteractionComponent->InteractableActionText = FText::FromString("Take");
	InteractionComponent->OnInteract.AddDynamic(this, &APickupBase::OnTakePickup);
	InteractionComponent->SetupAttachment(PickupMesh);

	// enable replication
	SetReplicates(true);
}

// create the pickups, set qty, rep item and mark dirty in the event a player drops an item back into the world
void APickupBase::InitializePickup(const TSubclassOf<class UBaseItem> ItemClass, const int32 Quantity) 
{
	if (HasAuthority() && ItemClass && Quantity > 0)
	{
		Item = NewObject<UBaseItem>(this, ItemClass);
		Item->SetQuantity(Quantity);

		OnRep_Item();
		Item->MarkDirtyForReplication();
	}
}

void APickupBase::OnRep_Item() 
{
	if (Item)
	{
		// bind the static mesh and display name to the pickup object
		PickupMesh->SetStaticMesh(Item->PickupMesh);
		InteractionComponent->InteractableNameText = Item->ItemDisplayName;

		// binds clients to the delegate for UI refresh when quantities change
		Item->OnItemModified.AddDynamic(this, &APickupBase::OnItemModified);
	}

	// if properties are changed, refresh widget
	InteractionComponent->RefreshWidget();
}

void APickupBase::OnItemModified() 
{
	if (InteractionComponent)
	{
		// if properties are changed, refresh widget
		InteractionComponent->RefreshWidget();
	}
}

// Called when the game starts or when spawned
void APickupBase::BeginPlay()
{
	Super::BeginPlay();
	
	// only server should call initalize pickup on item templates at startup
	if (HasAuthority() && ItemTemplate && bNetStartup)
	{
		// spawn the pickups with desired class and quantity
		InitializePickup(ItemTemplate->GetClass(), ItemTemplate->GetQuantity());
	}

	// if not at startup, indicating a player dropped an item, align pickup with the ground
	if (!bNetStartup)
	{
		AlignWithGround();
	}

	// if item, mark dirty for rep so players know about the item
	if (Item)
	{
		Item->MarkDirtyForReplication();
	}
}

void APickupBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// replicates and makes the server manage the quantity value
    DOREPLIFETIME(APickupBase, Item);
}

bool APickupBase::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) 
{
	// true when a modification has been made to actor channel
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// replicate if item marked as needs to replicate
	if (Item && Channel->KeyNeedsToReplicate(Item->GetUniqueID(), Item->RepKey))
	{
		bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}

	// returns a bool
	return bWroteSomething;
}

#if WITH_EDITOR
	void APickupBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

    // find the name of the property that was changed
    FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// If a new pickup is selected in the property editor, change the mesh to reflect the new item
    if (PropertyName == GET_MEMBER_NAME_CHECKED(APickupBase, ItemTemplate))
    {
		if (ItemTemplate)
		{
			// set the pickup to the new static mesh
			PickupMesh->SetStaticMesh(ItemTemplate->PickupMesh);
		}
    }
}
#endif

void APickupBase::OnTakePickup(class AMainCharacter* Taker) 
{
	// check the player is valid, log if it is not
	if (!Taker)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pickup was taken, but player not valid"));
		return;
	}

	// check that is server, item wasnt taken from a player that was killed, valid item
	if(HasAuthority() && !IsPendingKillPending() && Item)
	{
		// if players inventory is valid
		if (UInventoryComponent* PlayerInventory = Taker->PlayerInventory)
		{
			// check what the add result was
			const FItemAddResult AddResult = PlayerInventory->TryAddItem(Item);

			// if actual amount taken was less than total quantity, set quantity for pickup to new value
			if (AddResult.ActualAmountGiven < Item->GetQuantity())
			{
				Item->SetQuantity(Item->GetQuantity() - AddResult.ActualAmountGiven);
			}
			// if it was more than or equal to total pickup quantity, destroy the pickup from the world
			else if (AddResult.ActualAmountGiven >= Item->GetQuantity())
			{
				Destroy();
			}
		}
	}
}