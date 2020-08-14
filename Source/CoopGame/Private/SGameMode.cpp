// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameMode.h"
#include "TimerManager.h"
#include "Components/SHealthComponent.h"
#include "EngineUtils.h"
#include "SPlayerState.h"
#include "SGameState.h"
#include "Kismet/GameplayStatics.h"

ASGameMode::ASGameMode()
{
    TimeBetweenWaves = 2.0f;


    CurrentBossNumber = 0;

    GameStateClass = ASGameState::StaticClass();

    PlayerStateClass = ASPlayerState::StaticClass();

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;//Tick Once a second

}


void ASGameMode::StartWave()
{   
    WaveCount++;

    NrOfBotsToSpawn = 2*WaveCount;

    BossLimit = WaveCount/BossStartToAppearRound;

    UGameplayStatics::PlaySound2D(this,StartSound);

    GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.0f, true, 0.0f);

    SetWaveState(EWaveState::WaveInProgress);
}

void ASGameMode::EndWave()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
    
    UGameplayStatics::PlaySound2D(this,EndSound);

    SetWaveState(EWaveState::WaitingToComplete);
}

void ASGameMode::PrepareForNextWave()
{
    
    UGameplayStatics::PlaySound2D(this,PrepareSound);
    
    GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimeBetweenWaves, false);

    SetWaveState(EWaveState::WaitingToStart);

    CurrentBossNumber = 0;

    RestartDeadPlayers();//revive player who's dead;
}

void ASGameMode::CheckWaveState()
{   
    bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);


    if(NrOfBotsToSpawn > 0 || bIsPreparingForWave) //still spawning bots or is preparing next wave, don't run the logic below.
    {
        return;
    }
    bool bIsAnyBotAlive = false;

    for(TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* TestPawn = *It;
        if(TestPawn == nullptr || TestPawn->IsPlayerControlled())
        {
            continue; //we don't care about people, we only care about the AI;
        }

        USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
        if(HealthComp && HealthComp->GetHealth()>0.0f)//still alive
        {
            bIsAnyBotAlive = true;
            break;
        }
    }

    if(!bIsAnyBotAlive)
    {   
        SetWaveState(EWaveState::WaveComplete);
        PrepareForNextWave();
    }
}

void ASGameMode::CheckAnyPlayerAlive()
{
    for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if(PC && PC->GetPawn())
        {
            APawn* MyPawn = PC->GetPawn();
            USHealthComponent* HealthComp = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));
            if(ensure(HealthComp) && HealthComp->GetHealth()>0.0f)
            {
                //a player is still alive
                return;
            }
        }
    }

    //No player alive
    GameOver();    
}

void ASGameMode::GameOver()
{
    EndWave();
    //@TODO finidh up the match presernt the "game over" to players.
    SetWaveState(EWaveState::GameOver);

    
}

void ASGameMode::StartPlay()
{
    Super::StartPlay();

    PrepareForNextWave();
}

void ASGameMode::SpawnBotTimerElapsed()
{
    MonsterSelection();
    
    SpawnNewBot();

    NrOfBotsToSpawn--;

    if(NrOfBotsToSpawn<=0)
    {
        EndWave();
    }

}

void ASGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    CheckWaveState();

    CheckAnyPlayerAlive();
}

void ASGameMode::SetWaveState(EWaveState NewState)
{
    ASGameState* GS = GetGameState<ASGameState>();
    if(ensureAlways(GS))
    {
        GS->SetWaveState(NewState);
        
    }
}

void ASGameMode::RestartDeadPlayers()
{
     for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if(PC && PC->GetPawn() == nullptr)
        {
            RestartPlayer(PC);
        }   
    }
}

void ASGameMode::MonsterSelection()
{
    if(CurrentBossNumber<BossLimit)
    {
        GenerateIndex = FMath::RandRange(0,2);
        if(GenerateIndex==2)
        {
            CurrentBossNumber++;
            
        }
        return;
    }

    GenerateIndex = FMath::RandRange(0,1);

}