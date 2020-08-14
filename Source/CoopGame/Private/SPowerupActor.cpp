// Fill out your copyright notice in the Description page of Project Settings.


#include "SPowerupActor.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASPowerupActor::ASPowerupActor()
{
	PowerupInterval = 0.0f;
	TotalNrOfTicks = 0;

	bIsPowerupActive = false;

	SetReplicates(true);

}




void ASPowerupActor::OnTickPowerup()
{
	TickProcessed++;
	OnPowerupTicked();


	if(TickProcessed >= TotalNrOfTicks )
	{
		OnExpired();

		bIsPowerupActive = false;
		OnRep_PowerupActive();//this is to let the function run on the server, becasue the bIsPowerupActive will tirgger the client 
		// Delete Timer
		GetWorldTimerManager().ClearTimer(TimerHandle_PowerupTick);
	}
}

void ASPowerupActor::ActivatePowerup(AActor* ActivateFor)
{
	OnActivated(ActivateFor);
	
	bIsPowerupActive = true;
	OnRep_PowerupActive();//this is to let the function run on the server, becasue the bIsPowerupActive will tirgger the client 

	if(PowerupInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_PowerupTick, this, &ASPowerupActor::OnTickPowerup, PowerupInterval, true);
	}
	else
	{
		OnTickPowerup();
	}
}

void ASPowerupActor::OnRep_PowerupActive()
{
	OnPowerupStateChanged(bIsPowerupActive);	
}

//Let us specify how we want to replicate and what do we want to replicate (got this from weird place)
void ASPowerupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);


	DOREPLIFETIME(ASPowerupActor, bIsPowerupActive);
}