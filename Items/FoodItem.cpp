// Fill out your copyright notice in the Description page of Project Settings.


#include "FoodItem.h"

// namespace used for language localization
#define LOCTEXT_NAMESPACE "Item"

// set default food properties
UFoodItem::UFoodItem() 
{
    HealthToHeal = 5.f;
    StaminaToRecover = 5.f;
    HungerToRecover = 5.f;
    ThirstToRecover = 5.f;

    UseActionText = LOCTEXT("ItemUseActionText", "Consume");
}

void UFoodItem::Use(class AMainCharacter* Character) 
{
    // change health, stamina, hunger and thirst here
    
    // print to log
    //UE_LOG(LogTemp, Warning, TEXT("Ate food"));
}

#undef LOCTEXT_NAMESPACE