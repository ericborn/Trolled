// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionWidget.h"
#include "Components/InteractionComponent.h/"


void UInteractionWidget::UpdateInteractionWidget(class UInteractionComponent* InteractionComponent) 
{
    // sets the owner to the value passed in
    OwningInteractionComponent = InteractionComponent;

    // call the update function
    OnUpdateInteractionWidget();
}

void UInteractionWidget::OnUpdateInteractionWidget() 
{
    
}
