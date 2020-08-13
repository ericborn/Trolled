// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

/**
 * 
 */
UCLASS()
class TROLLED_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	// allows updates to c++ card
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UpdateInteractionWidget(class UInteractionComponent* InteractionComponent);

	// Allows updates to the bp card
	UFUNCTION(BlueprintImplementableEvent)
	void OnUpdateInteractionWidget();

	// interaction component that the card uses
	UPROPERTY(BlueprintReadOnly, Category = "Interaction", meta = (ExposeOnSpawn))
	class UInteractionComponent* OwningInteractionComponent;
	
};
