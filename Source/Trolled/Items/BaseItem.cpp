// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseItem.h"
#include "Trolled/Components/InventoryComponent.h"
#include "Net/UnrealNetwork.h"

// namespace used for language localization
#define LOCTEXT_NAMESPACE "Item"

void UBaseItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const 
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // replicates and makes the server manage the quantity value
    DOREPLIFETIME(UBaseItem, Quantity);
}

bool UBaseItem::IsSupportedForNetworking() const 
{
    // Uobjects are not networked by default
    return true;
}

class UWorld* UBaseItem::GetWorld() const 
{
    return World;
}

#if WITH_EDITOR
    void UBaseItem::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) 
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // find the name of the property that was changed
    FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

    // UPROPERTY doesn't support using a variable to clamp, so its done here
    if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UBaseItem, Quantity))
    {
        // sets quantity of item being set in the BP to follow the max stack size defined at the base item level
        Quantity = FMath::Clamp(Quantity, 1, bStackable ? MaxStackSize : 1);
    }
}
#endif

// setup item default values
UBaseItem::UBaseItem() 
{
    // loctext takes a key than translation. UE has dashboard for mapping key to different language values
    ItemDisplayName = LOCTEXT("ItemName", "Item");
    UseActionText = LOCTEXT("ItemUseActionText", "Use");
    Weight = 0.f;
    bStackable = true;
    Quantity = 1;
    MaxStackSize = 50;
    RepKey = 0;
}

void UBaseItem::OnRep_Quantity() 
{
    OnItemModified.Broadcast();
}

void UBaseItem::SetQuantity(const int32 NewQuantity) 
{
    if(NewQuantity != Quantity)
    {   
        // clamp NewQuantity quant min at 0 to prevent negative, up to max stack size if its stackable, otherwise max is 1
        Quantity = FMath::Clamp(NewQuantity, 0, bStackable ? MaxStackSize : 1);
        MarkDirtyForReplication();
    }
}

bool UBaseItem::ShouldShowInInventory() const
{
    return true;
}

void UBaseItem::Use(class AMainCharacter* Character) 
{
    
}

void UBaseItem::AddedToInventory(class UInventoryComponent* Inventory) 
{
    
}

void UBaseItem::MarkDirtyForReplication() 
{
    // mark object for replication
    ++RepKey;

    // mark inventory array for replication
    if(OwningInventory)
    {
        ++OwningInventory->ReplicatedItemsKey;
    }
}

#undef LOCTEXT_NAMESPACE