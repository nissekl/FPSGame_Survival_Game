// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameState.h"
#include "Net/UnrealNetwork.h"
void ASGameState::OnRep_WaveState(EWaveState OldState)
{
    WaveStateChanged(WaveState, OldState);
}


//Let us specify how we want to replicate and what do we want to replicate (got this from weird place)
void ASGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASGameState, WaveState);//replicate this variable to any client connect to us

}

void ASGameState::SetWaveState(EWaveState NewState)
{
    if(GetLocalRole() == ROLE_Authority)
    {
        EWaveState OldState = WaveState;
        
        WaveState = NewState;//trigger(the On_rep to run on client)
        //Call on server
        OnRep_WaveState(OldState);//this is on the server
    }
}