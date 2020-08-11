// Fill out your copyright notice in the Description page of Project Settings.


#include "TrolledPlayerController.h"
#include "Trolled/MainCharacter.h"
#include "Trolled/Framework/TrolledGameStateBase.h"
#include "Kismet/GameplayStatics.h"

ATrolledPlayerController::ATrolledPlayerController() 
{
}

void ATrolledPlayerController::BeginPlay() 
{
    Super::BeginPlay();
}

void ATrolledPlayerController::SetupInputComponent() 
{
    Super::SetupInputComponent();

    InputComponent->BindAxis("Turn", this, &ATrolledPlayerController::Turn);
	InputComponent->BindAxis("LookUp", this, &ATrolledPlayerController::LookUp);

	//InputComponent->BindAction("OpenInventory", IE_Pressed, this, &ATrolledPlayerController::OpenInventory);

	//InputComponent->BindAction("Pause", IE_Pressed, this, &ATrolledPlayerController::PauseGame);

	//InputComponent->BindAction("OpenMap", IE_Pressed, this, &ATrolledPlayerController::OpenMap);
	//InputComponent->BindAction("OpenMap", IE_Released, this, &ATrolledPlayerController::CloseMap);

	InputComponent->BindAction("Reload", IE_Pressed, this, &ATrolledPlayerController::StartReload);
}

void ATrolledPlayerController::Respawn() 
{
    // unpossess the controller from the current character
    UnPossess();

    // UE will only allow respawn while player is inactive
    // set to inactive so respawn allowed
	ChangeState(NAME_Inactive);

    // if not the server, ask the server to respawn
	if (!HasAuthority())
	{
		ServerRespawn();
	}
	else
	{
		// respawn player
        ServerRestartPlayer();
	}
}

void ATrolledPlayerController::ServerRespawn_Implementation() 
{
    Respawn();
}

bool ATrolledPlayerController::ServerRespawn_Validate()
{
    return true;
}

void ATrolledPlayerController::ApplyRecoil(const FVector2D& RecoilAmount, const float RecoilSpeed, const float RecoilResetSpeed, TSubclassOf<class UCameraShake> Shake) 
{
    // apply locally only
    if (IsLocalPlayerController())
	{
		// cam manager responsible for local camera
        if (PlayerCameraManager)
        {
            // apply shake
            PlayerCameraManager->PlayCameraShake(Shake);
        }

        // store current recoil amounts, processed in look/turn functions
        RecoilBumpAmount += RecoilAmount;
        RecoilResetAmount += -RecoilAmount;

        CurrentRecoilSpeed = RecoilSpeed;
        CurrentRecoilResetSpeed = RecoilResetSpeed;

        LastRecoilTime = GetWorld()->GetTimeSeconds();
	}
}

void ATrolledPlayerController::Turn(float Rate) 
{
    //If the player has moved their camera to compensate for recoil we need this to cancel out the recoil reset effect
	if (!FMath::IsNearlyZero(RecoilResetAmount.X, 0.01f))
	{

		if (RecoilResetAmount.X > 0.f && Rate > 0.f)
		{
			RecoilResetAmount.X = FMath::Max(0.f, RecoilResetAmount.X - Rate);
		}
		else if (RecoilResetAmount.X < 0.f && Rate < 0.f)
		{
			RecoilResetAmount.X = FMath::Min(0.f, RecoilResetAmount.X - Rate);
		}
	}

	//Apply the recoil over several frames
	if (!FMath::IsNearlyZero(RecoilBumpAmount.X, 0.1f))
	{
		FVector2D LastCurrentRecoil = RecoilBumpAmount;
		RecoilBumpAmount.X = FMath::FInterpTo(RecoilBumpAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		AddYawInput(LastCurrentRecoil.X - RecoilBumpAmount.X);
	}

	//Slowly reset back to center after recoil is processed
	FVector2D LastRecoilResetAmount = RecoilResetAmount;
	RecoilResetAmount.X = FMath::FInterpTo(RecoilResetAmount.X, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);
	AddYawInput(LastRecoilResetAmount.X - RecoilResetAmount.X);

	AddYawInput(Rate);
}

void ATrolledPlayerController::LookUp(float Rate) 
{
    // check if recoil amount is not 0, therefore recoil
    if (!FMath::IsNearlyZero(RecoilResetAmount.Y, 0.01f))
	{
        // up recoil
		if (RecoilResetAmount.Y > 0.f && Rate > 0.f)
		{
			// adjust the reset amount minus the amount a player has moved their mouse down to compensate
            RecoilResetAmount.Y = FMath::Max(0.f, RecoilResetAmount.Y - Rate);
		}

        // down recoil
		else if (RecoilResetAmount.Y < 0.f && Rate < 0.f)
		{
            // adjust the reset amount minus the amount a player has moved their mouse up to compensate
			RecoilResetAmount.Y = FMath::Min(0.f, RecoilResetAmount.Y - Rate);
		}
	}

    // Apply the recoil over several frames
	if (!FMath::IsNearlyZero(RecoilBumpAmount.Y, 0.01f))
	{
		FVector2D LastCurrentRecoil = RecoilBumpAmount;

        // interpolate toward 0 based on the recoil speed and time
		RecoilBumpAmount.Y = FMath::FInterpTo(RecoilBumpAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilSpeed);

		// moves the camera based upon the bump amount
        AddPitchInput(LastCurrentRecoil.Y - RecoilBumpAmount.Y);
	}

    //Slowly reset back to center after recoil is processed
	FVector2D LastRecoilResetAmount = RecoilResetAmount;
	RecoilResetAmount.Y = FMath::FInterpTo(RecoilResetAmount.Y, 0.f, GetWorld()->DeltaTimeSeconds, CurrentRecoilResetSpeed);
	AddPitchInput(LastRecoilResetAmount.Y - RecoilResetAmount.Y);

    // player mouse input with no recoil
	AddPitchInput(Rate);
}

// allows reload if alive, otherwise respawn
void ATrolledPlayerController::StartReload() 
{
   	if (AMainCharacter* MainCharacter = Cast<AMainCharacter>(GetPawn()))
	{
		if (MainCharacter->IsAlive())
		{
			MainCharacter->StartReload();
		}
		else // R key should respawn the player if dead
		{
			Respawn();
		}
	} 
}

// void ATrolledPlayerController::Died(class AMainCharacter* Killer) 
// {

// }

// void ATrolledPlayerController::Died(class ASurvivalCharacter* Killer) 
// {
//    	if (ATrolledGameState* GS = Cast<ATrolledGameState>(UGameplayStatics::GetGameState(GetWorld())))
// 	{
// 		//Force the player to respawn 
// 		FTimerHandle DummyHandle;
// 		GetWorldTimerManager().SetTimer(DummyHandle, this, &ATrolledPlayerController::Respawn, GS->RespawnTime, false);

// 		if (ASurvivalHUD* HUD = Cast<ASurvivalHUD>(GetHUD()))
// 		{
// 			HUD->ShowDeathWidget(Killer);
// 		}
// 	} 
// }

void ATrolledPlayerController::ClientShowNotification_Implementation(const FText& Message) 
{
    ShowNotification(Message);
}


