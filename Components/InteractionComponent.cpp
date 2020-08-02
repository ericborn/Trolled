// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionComponent.h"

UInteractionComponent::UInteractionComponent()
{
    // disable tick since we dont need to check every frame, only when interacting
    SetComponentTickEnabled(false);

    // interact time
    InteractionTime = 0.f;
    
    // interact distance 2m
    InteractionDistance = 200.f;

    // default name text
    InteractionNameText = FText::FromString("Interactable Object");

    // default interact text
    InteractionActionText = FText::FromString("Interact");

    // allow multiple interactors
    bAllowMultipleInteractors = true;

    // Setup the UI for displaying interaction text in screen space
    Space = EWidgetSpace::Screen;
    DrawSize = FIntPoint(400, 100);
    bDrawAtDesiredSize = true;

    // enabled but hidden by default to prevent every interaction from displaying on spawn in
    SetActive(true);
    SetHiddenInGame(true);
}


void UInteractionComponent::StartFocus(class AMainCharacter* Character) 
{
    // if the component isnt active, doesnt have an owner or the character is null, just return from function
    if (!IsActive() || !GetOwner() || !Character)
    {
        return;
    }

    // binds to the delegate when a player looks at the interactable
    OnStartFocus.Broadcast(Character);

    // show UI
    SetHiddenInGame(false);

    // if not the server
    if (!GetOwner()->HasAuthority())
    {
        // grab visual components, primitive
        for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass()))
        {
            // set outline around the object
            if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
            {
                Prim->SetRenderCustomDepth(true);
            }
        }  
    }
    
}

void UInteractionComponent::StopFocus(class AMainCharacter* Character) 
{
    
}

void UInteractionComponent::StartInteract(class AMainCharacter* Character) 
{
    
}

void UInteractionComponent::StopInteract(class AMainCharacter* Character) 
{
    
}

void UInteractionComponent::Interact(class AMainCharacter* Character) 
{
    OnInteract.Broadcast(Character);
}