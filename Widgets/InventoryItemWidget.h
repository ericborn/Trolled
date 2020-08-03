// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemWidget.generated.h"

/**
 * 
 */
UCLASS()
class TROLLED_API UInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// reference to the item it contains, expose allows bp to hook to this value on spawn
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item Widget", meta = (ExposeOnSpawn = true))
	class UBaseItem* Item;
	
};
