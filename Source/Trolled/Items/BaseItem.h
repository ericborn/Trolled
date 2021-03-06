// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BaseItem.generated.h"

// Delegate used to update the UI anytime items are modified
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemModified);

UENUM(BlueprintType)
enum class EItemRarity : uint8
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

protected:
	//networking for UObject
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;
	
	// item needs a reference to the world for bp's to work correctly
	virtual class UWorld* GetWorld() const override;

// allows changing items while the game is in the editor, when published the ability to modify items is removed
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UBaseItem();

	UPROPERTY(Transient)
	class UWorld* World;

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item", meta = (ClampMin = 2, EditCondition = bStackable))
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

	// function for replicating quantity changes
	UFUNCTION()
	void OnRep_Quantity();

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(const int32 NewQuantity);

	UFUNCTION(BlueprintPure, Category = "Item")
	FORCEINLINE int32 GetQuantity() const { return Quantity; };

	// key used to efficiently replicate inventory items between client/server
	// when key changes, server knows to send updated to client
	UPROPERTY()
	int32 RepKey;

	// accessed by the delegate for inventory UI updates
	UPROPERTY(BlueprintAssignable)
	FOnItemModified OnItemModified;

	// function for calculating stack weight, quantity * weight of single item
	UFUNCTION(BlueprintCallable, Category = "Item")
	FORCEINLINE float GetStackWeight() const { return Quantity * Weight; };

	// function to hide/show items in the inventory
	UFUNCTION(BlueprintPure, Category = "Item")
	virtual bool ShouldShowInInventory() const;

	// expose the base item to blueprints so items and be quickly created and modified using BP's
	UFUNCTION(BlueprintImplementableEvent)
	void OnUse(class AMainCharacter* Character);

	// use function, can override within the item to change how the use is implemented
	virtual void Use(class AMainCharacter* Character);

	// item added to inventory function, can be overridden to change how the use is implemented, equip directly, put in inventory, etc.
	virtual void AddedToInventory(class UInventoryComponent* Inventory);

	// Used to make an object that needs replicating. Must be called internally after modifying any replicated properties
	void MarkDirtyForReplication();
};

