// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

UCLASS()
class TROLLED_API APickupBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupBase();

	// The item pickup that sits in the world. Initialized on BeginPlay and when a player drops it from inventory
	void InitializePickup(const, TSubclassOf<class UBaseItem> ItemClass, const int32 Quantity);

	// helper to align the pickup mesh with the grounds rotation, declared in cpp but implemented in BP's
	UFUNCTION(BlueprintImplementableEvent)
	void AlignWithGround();

	// Instanced template that holds the actual items info
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	class UBaseItem* ItemTemplate;

protected:

	// The item that is added to the inventory when pickup is taken
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, ReplicatedUsing = OnRep_Item)
	class UBaseItem* Item;

	// replication function
	UFUNCTION()
	void OnRep_Item();

	// binds to the UI for updates when an item is modified
	UFUNCTION()
	void OnItemModified();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// pickup items are replicated by the server to all players so they know where/how much of an item exists in the world
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicatedSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

// allows changing items while the game is in the editor, when published the ability to modify items is removed
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// called when a player takes the pickup from the world
	UFUNCTION()	
	void OnTakePickup(class AMainCharacter* Taker);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")	
	class UStaticMeshComponent* PickupMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Components")	
	class UInteractionComponent* InteractionComponent;

};