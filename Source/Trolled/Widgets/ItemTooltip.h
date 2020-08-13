// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemTooltip.generated.h"

/**
 * 
 */
UCLASS()
class TROLLED_API UItemTooltip : public UUserWidget
{
	GENERATED_BODY()

public:

	// reference to the item widget
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip", meta = (ExposeOnSpawn = true))
	class UInventoryItemWidget* InventoryItemWidget;
	
};
