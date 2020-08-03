// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseItem.h"
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
    
}

bool UBaseItem::ShouldShowInInventory() const
{
    return true;
}

void UBaseItem::MarkDirtyForReplication() 
{
    
}

#undef LOCTEXT_NAMESPACE