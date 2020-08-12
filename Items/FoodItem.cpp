// Fill out your copyright notice in the Description page of Project Settings.

#include "FoodItem.h"
#include "Trolled/MainCharacter.h"
#include "Trolled/Player/TrolledPlayerController.h"
#include "Trolled/Components/InventoryComponent.h"

// namespace used for language localization
#define LOCTEXT_NAMESPACE "FoodItem"

// set default food properties
UFoodItem::UFoodItem() 
{
    HealAmount = 5.f;
    StaminaAmount = 5.f;
    HungerAmount = 5.f;
    ThirstAmount = 5.f;

    UseActionText = LOCTEXT("ItemUseActionText", "Consume");
}

void UFoodItem::Use(class AMainCharacter* Character) 
{
    // if character valid
    if (Character)
	{
		// find health, stamina, hunger and thirst recovery amount
        const float ActualHealedAmount = Character->ModifyHealth(HealAmount);
        const float ActualStaminaAmount = Character->ModifyStamina(StaminaAmount);
        const float ActualHungerAmount = Character->ModifyHunger(HungerAmount);
        const float ActualThirstAmount = Character->ModifyThirst(ThirstAmount);

        // check if didnt recovered any stats
		const bool bHealed = !FMath::IsNearlyZero(ActualHealedAmount);
		const bool bStamina = !FMath::IsNearlyZero(ActualStaminaAmount);
        const bool bHunger = !FMath::IsNearlyZero(ActualHungerAmount);
		const bool bThirst = !FMath::IsNearlyZero(ActualThirstAmount);

        // if not the server, display a message that the food was consumed
		if (!Character->HasAuthority())
		{
			if (ATrolledPlayerController* PC = Cast<ATrolledPlayerController>(Character->GetController()))
			{
				if (bHealed || bStamina || bHunger || bThirst)
				{
					PC->ClientShowNotification(FText::Format(LOCTEXT("ConsumeText", "Ate {FoodName}, healed {HealAmount} health."), ItemDisplayName, ActualHealedAmount));
				}
				else
				{
					PC->ClientShowNotification(FText::Format(LOCTEXT("FullHealthText", "No need to eat {FoodName}, health is already full."), ItemDisplayName, ActualHealedAmount));
				}
			}
		}

        // if food used, remove from inventory on client so the use is instant
		if (bHealed || bStamina || bHunger || bThirst)
		{
			if (UInventoryComponent* Inventory = Character->PlayerInventory)
			{
				Inventory->ConsumeQuantity(this, 1);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE