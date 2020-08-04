// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupBase.h"
#include "Trolled/Items/BaseItem.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Trolled/Components/InteractionComponent.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Engine/ActorChannel.h"

// Sets default values
APickupBase::APickupBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// create the pickups, set qty, rep item and mark dirty in the event a player drops an item back into the world
void APickupBase::InitializePickup(const, TSubclassOf<class UBaseItem> ItemClass, const int32 Quantity) 
{
	if (HasAuthority() && ItemClass && Quantity > 0)
	{
		Item = NewObject<UItem>(this, ItemClass);
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
		Item->OnItemModified.AddDynamic(this, &APickup::OnItemModified);
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
		IntializePickup(ItemTemplate->GetClass(), ItemTemplate->GetQuantity());
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
    DOREPLIFETIME(UPickupBase, BaseItem);
}

bool APickupBase::ReplicatedSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) 
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
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UPickupBase, ItemTemplate))
    {
		if (ItemTemplate)
		{
			// set the pickup to the new static mesh
			PickupMesh->SetStaticMesh(ItemTemplate->PickupMesh);
		}
    }
}

void APickupBase::OnTakePickup(class AMainCharacter* Taker) 
{
	
}
#endif