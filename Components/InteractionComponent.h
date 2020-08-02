// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartInteract, class AMainCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStopInteract, class AMainCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartFocus, class AMainCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStopFocus, class AMainCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, class AMainCharacter*, Character);

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TROLLED_API UInteractionComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	// time it takes to fully interact with object
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionTime;

	// Max distance a player needs to be to interact with the object
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionDistance;

	// Name text of the object when looking at it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionNameText;

	// Action text of how to use the object when looking at it
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionActionText;

	// Whether mulitple players can interact at one time
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bAllowMultipleInteractors;

	// allows dynamic changes to the interactables name and action text (on/off, open/close) depending on state
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractableNameText(const FText& NewNameText);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractableActionText(const FText& NewActionText);


	// Create delegates that bind to the interactable, allows each interact item to have different behaviors, with the same base functionality
	// [Local + Server] Called when the player presses the interact button while looking at this interactable actor
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnStartInteract OnStartInteract;

	// [Local + Server] Called when the player releases the interact button, stops looking at this interactable actor or moves too far away
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnStopInteract OnStopInteract;

	// [Local + Server] Called when the player looks at this interactable actor
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnStartFocus OnStartFocus;

	// [Local + Server] Called when the player stops looks at this interactable actor, or moves too far away
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnStopFocus OnStopFocus;

	// [Local + Server] Called when the player has interacted with this interactable actor
	UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
	FOnInteract OnInteract;

protected:
	
	// called at game start to clear all interactions
	virtual void Deactivate() override;

	// check if a character can interact with a specific object
	bool CanInteract(class AMainCharacter* Character) const;

	// server side array to store all characters interacting with an item, client side only stores self
	UPROPERTY()
	TArray<class AMainCharacter*> Interactors;


public:
	// Called on the client when a player interaction check trace starts or stops hitting this item
	void StartFocus(class AMainCharacter* Character);
	void StopFocus(class AMainCharacter* Character);

	// Called on the client when a player starts or stops interacting with this item
	void StartInteract(class AMainCharacter* Character);
	void StopInteract(class AMainCharacter* Character);

	// called when the interaction is performed
	void Interact(class AMainCharacter* Character);

	// returns value between 0-1 for progress through the interaction, used for interaction progress bar
	// server side is the first interactors percentage, client side is the local interactors percentage
	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetInteractPercentage();
};
