// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseItem.generated.h"

UENUM(BlueprintType)
enum class EItemRarity : unit8
{
	IR_Common UMETA(DisplayName = "Common"),
	IR_Uncommon UMETA(DisplayName = "Uncommon"),
	IR_Rare UMETA(DisplayName = "Rare"),
	IR_VeryRare UMETA(DisplayName = "Very Rare"),
	IR_Legendary UMETA(DisplayName = "Legendary")
};

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class TROLLED_API UBaseItem : public UObject
{
	GENERATED_BODY()

public:
	UBaseItem();

	// item mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UStaticMesh* PickupMesh;

	// item thumbnail
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	class UTexture2D* Thumbnail;

	// Display name for the item
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText ItemDisplayName;

	// item description, multiline allows multiple lines of text
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (MultiLine = true))
	FText ItemDescription;

	// text for how the item is used
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FText UseActionText;

	// item Rarity
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity;

	// item weight, clamped to a minimum of 0 to prevent item weighing negative
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 0.0))
	float Weight;

	// is the item stackable
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	bool bStackable;

	// Max stack size
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 2.0), EditCondition = bStackable))
	int32 MaxStackSize;

	// item tooltip
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	TAssetSubclassOf<class UItemTooltip> ItemTooltip;

	// current item amount, stored on the server to prevent cheating
	UPROPERTY(ReplicatedUsing = OnRep_Quantity, EditAnywhere, Category = "Item", meta = (UIMin = 1, EditCondition = bStackable))
	int32 Quantity;

	// Inventory that owns the item
	UPROPERTY()
	class UInventoryComponent* OwningInventory;

	// function for calculating stack weight, quantity * weight of single item
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; };

	// function to hide/show items in the inventory
	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool ShouldShowInInventory() const;
};

