// Fill out your copyright notice in the Description page of Project Settings.


#include "FoodItem.h"

// namespace used for language localization
#define LOCTEXT_NAMESPACE "Item"

// set default food properties
UFoodItem::UFoodItem() 
{
    HealthAmount = 5.f;
    StaminaAmount = 5.f;
    HungerAmount = 5.f;
    ThirstAmount = 5.f;

    UseActionText = LOCTEXT("ItemUseActionText", "Consume");
}

void UFoodItem::Use(class AMainCharacter* Character) 
{
    // change health, stamina, hunger and thirst here
}

#undef