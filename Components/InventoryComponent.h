// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/BaseItem.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

// enum for selecting if the add item operation was not, partially or fully successful
UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NoItemsAdded UMETA(DisplayName = "No items added"),
	IAR_SomeItemsAdded UMETA(DisplayName = "Some items added"),
	IAR_AllItemsAdded UMETA(DisplayName = "All items added")
};

// create a struct and add it to bp
USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()

public:

	// constructors
	FItemAddResult() {};
	FItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), ActualAmountGiven(0) {};
	FItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), ActualAmountGiven(InQuantityAdded) {};

	// amount trying to be added
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 AmountToGive;

	// amount actually added
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 ActualAmountGiven;

	// result of the attempt
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	EItemAddResult Result;

	// if there was an error, what was it
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText ErrorText;

	// helpers
	// no items added
	static FItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText)
	{
		FItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = EItemAddResult::IAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;

		return AddedNoneResult;
	}

	// some items added
	static FItemAddResult AddedSome(const int32 InItemQuantity, const int32 ActualAmountGiven, const FText& ErrorText)
	{
		FItemAddResult AddedSomeResult(InItemQuantity, ActualAmountGiven);

		AddedSomeResult.Result = EItemAddResult::IAR_SomeItemsAdded;
		AddedSomeResult.ErrorText = ErrorText;

		return AddedSomeResult;
	}

	// All items added
	static FItemAddResult AddedAll(const int32 InItemQuantity)
	{
		FItemAddResult AddedAllResult(InItemQuantity, InItemQuantity);

		AddedAllResult.Result = EItemAddResult::IAR_AllItemsAdded;

		return AddedAllResult;
	}

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TROLLED_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	// allows base item to access private functions inside of inventory component
	friend class UBaseItem;

	/** Add item to inventory
	 * @param ErrorText the text to display if the item couldnt be added
	 * @return the amount of the item that was added to the inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItem(class UBaseItem* Item);

	/** Add item to inventory using items class instead of item instance
	 * @param ErrorText the text to display if the item couldnt be added
	 * @return the amount of the item that was added to the inventory */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItemFromClass(TSubclassOf<class UBaseItem> ItemClass, const int32 Quantity);

	// used for enabling blueprints(UI) to use these functions
	UFUNCTION(BlueprintPure,  Category = "Inventory")
	FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; };
	
	UFUNCTION(BlueprintPure,  Category = "Inventory")
	FORCEINLINE int32 GetInventoryCapacity() const { return InventoryCapacity; };
	
	UFUNCTION(BlueprintPure,  Category = "Inventory")
	FORCEINLINE TArray<class UBaseItem*> GetItems() const { return InventoryArray; };

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
	// array for the current inventory, replicated to server
	UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, Category = "Inventory")
	TArray<class UBaseItem*> InventoryArray;

	// total inventory slots, increased with bags
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0, ClampMax = 200))
	int32 InventoryCapacity;

	// weight capacity
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	float WeightCapacity;

	//networking for UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags);


private:
	// used to refresh UI when items gained, used, equipped, etc
	UFUNCTION()
	void OnRep_Items();

	// used to control when an item needs to be replicated
	UPROPERTY()
	int32 ReplicatedItemsKey;

	// Internal, non-BP exposed add item function. Not used directly, call TryAddItem() or TryAddItemFromClass()
	FItemAddResult TryAddItem_Internal(class UBaseItem* Item);
		
};
