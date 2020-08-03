// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Trolled/Items/BaseItem.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TROLLED_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

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
		
};
