// Fill out your copyright notice in the Description page of Project Settings.


#include "EquippableItem.h"


void UEquippableItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquippableItem, bEquipped);
}
