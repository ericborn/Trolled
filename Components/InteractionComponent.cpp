// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionComponent.h"
#include "Trolled/Widgets/InteractionWidget.h"
#include "Trolled/MainCharacter.h"

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

void UInteractionComponent::SetInteractableNameText(const FText& NewNameText) 
{
    // changes the text then refreshes the widget to update the UI
    InteractionNameText = NewNameText;
    RefreshWidget();
}

void UInteractionComponent::SetInteractableActionText(const FText& NewActionText) 
{
    // changes the text then refreshes the widget to update the UI
    InteractionActionText = NewActionText;
    RefreshWidget();
}

// find all interactors, if they are characters, stop focus and interact
void UInteractionComponent::Deactivate() 
{
    Super::Deactivate();

    for (int32 i = Interactors.Num() - 1; i >= 0; --i)
    {
        if(AMainCharacter* Interactor = Interactors[i])
        {
            StopFocus(Interactor);
            StopInteract(Interactor);
        }
    }

    // Clear all actors
    Interactors.Empty();
}

bool UInteractionComponent::CanInteract(class AMainCharacter* Character) const
{
    // if multiple interactors is not allowed and the number of interactors is greater or equal to 1
    const bool bPlayerAlreadyInteracting = !bAllowMultipleInteractors && Interactors.Num() >= 1;
    
    // return not interacting, is active, with a valid owner and character
    return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}

void UInteractionComponent::RefreshWidget() 
{
    // if object not hidden and not the server
    if (!bHiddenInGame && GetOwner()->GetNetMode() != NM_DedicatedServer)
    {
        // check for widget to exist, then update it
        if (UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject()))
        {
            InteractionWidget->UpdateInteractionWidget(this);
        }
    }
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
    // binds to the delegate when a player looks at the interactable
    OnStopFocus.Broadcast(Character);

    // show UI
    SetHiddenInGame(true);

    // if not the server
    if (!GetOwner()->HasAuthority())
    {
        // grab visual components, primitive
        for (auto& VisualComp : GetOwner()->GetComponentsByClass(UPrimitiveComponent::StaticClass()))
        {
            // set outline around the object
            if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
            {
                Prim->SetRenderCustomDepth(false);
            }
        }  
    }
}

void UInteractionComponent::StartInteract(class AMainCharacter* Character) 
{
    if (CanInteract(Character))
    {
        // adds the player to list of interactors
        Interactors.AddUnique(Character);

        // calls delegate to start the interact
        OnStartInteract.Broadcast(Character);
    }
}

void UInteractionComponent::StopInteract(class AMainCharacter* Character) 
{
    // remove the player from list of interactors
    Interactors.RemoveSingle(Character);
    
    // calls delegate to stop the interact
    OnStopInteract.Broadcast(Character);
}

void UInteractionComponent::Interact(class AMainCharacter* Character) 
{
    if (CanInteract(Character))
    {
        OnInteract.Broadcast(Character);
    }
}

// Checks for a player to be in index 0 of the interactors array
// returns 1 minus the remaining time divided by interaction time
float UInteractionComponent::GetInteractPercentage() 
{
    if (Interactors.IsValidIndex(0))
    {
        if (AMainCharacter* Interactor = Interactors[0])
        {
            if (Interactor && Interactor->IsInteracting())
            {
                return 1.f - FMath::Abs(Interactor->GetRemainingInteractTime() / InteractionTime);
            }
        }
    }
    return 0.f;
}