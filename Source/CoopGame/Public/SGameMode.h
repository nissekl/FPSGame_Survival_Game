// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SGameMode.generated.h"

enum class EWaveState:uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilledSignature, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);


/**
 * 
 */
UCLASS()
class COOPGAME_API ASGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	//Hook for BP to spawn a single bot
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
	void SpawnNewBot();

	//Start Spawning bots
	void StartWave();

	//Stop Spawning bots
	void EndWave();
	
	//Set timer for next startwave
	void PrepareForNextWave();

	void SpawnBotTimerElapsed();

	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void GameOver();

	void SetWaveState(EWaveState NewState);

	void RestartDeadPlayers();

	void MonsterSelection();
protected:
	
	FTimerHandle TimerHandle_BotSpawner;

	FTimerHandle TimerHandle_NextWaveStart;

	int32 CurrentBossNumber;

	int32 BossLimit;

	//Bots to spwan in current wave
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	int32 NrOfBotsToSpawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	int32 GenerateIndex;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode")
	int32 WaveCount;

	UPROPERTY(EditDefaultsOnly, Category="GameMode")
	int32 BossStartToAppearRound;

	UPROPERTY(EditDefaultsOnly, Category = "GameMode")
	float TimeBetweenWaves;

	UPROPERTY(EditDefaultsOnly, Category="GameMode")
	USoundBase* PrepareSound;

	UPROPERTY(EditDefaultsOnly, Category="GameMode")
	USoundBase* StartSound;

	UPROPERTY(EditDefaultsOnly, Category="GameMode")
	USoundBase* EndSound;

	

	
public:
	ASGameMode();

	virtual void StartPlay() override;

	virtual void Tick(float DeltaSeconds) override;	

	UPROPERTY(BlueprintAssignable, Category="GameMode")
	FOnActorKilledSignature OnActorKilled;

	

};
