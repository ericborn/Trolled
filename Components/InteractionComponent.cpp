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