// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Components/SHealthComponent.h"
#include "SCharacter.h"
#include "Components/SphereComponent.h"
#include "Sound/SoundCue.h"
#include "EngineUtils.h"

static int32 DebugTrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackerBotDrawing(TEXT("COOP.DebugTrackerBot"), DebugTrackerBotDrawing, TEXT("Draw Debug Lines for Trackerbot"), ECVF_Cheat);

// Sets default values
ASTrackerBot::ASTrackerBot()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	//Use static mesh is due to we don't have to animate anything.
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	BotHealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("BotHealthComp"));
	BotHealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);

	bUseVelocityChange = false;

	MovementForce = 1000;

	RequiredDistanceToTarget = 100;

	ExplosionRadius = 350;

	ExplosionDamage = 60;

	SelfDamageInterval = 0.25f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		//Find initial move to
		NextPathPoint = GetNextPathPoint();

		//Every second we update our power level based on nearby bots
		FTimerHandle TimerHandle_CheckPowerLevel;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckPowerLevel, this, &ASTrackerBot::OnCheckNearbyBots, 1.0f, true);
	}
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority && !bExploded)
	{
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistanceToTarget) //close enough, keep updating new point
		{
			NextPathPoint = GetNextPathPoint();
			if(DebugTrackerBotDrawing)
			{
				DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached!");
			}
		}
		else
		{
			//Keep moveing toward next target
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();
			ForceDirection *= MovementForce;

			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);
			if(DebugTrackerBotDrawing)
			{	
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Green, false, 0.0f, 0, 1.0f);
			}
		}
		if(DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Green, false, 0.0f, 1.0f);
		}
	}
}

FVector ASTrackerBot::GetNextPathPoint()
{
	AActor *BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn *TestPawn = *It;
		if (TestPawn == nullptr || USHealthComponent::IsFriendly(TestPawn, this))
		{
			continue; //is the same team we continue;
		}

		USHealthComponent *TestPawnHealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (TestPawnHealthComp && TestPawnHealthComp->GetHealth() > 0.0f) //still alive
		{
			float Distance = (TestPawn->GetActorLocation() - GetActorLocation()).Size();

			if (Distance < NearestTargetDistance)
			{
				BestTarget = TestPawn;
				NearestTargetDistance = Distance;
			}
		}
	}
	if (BestTarget)
	{
		UNavigationPath *NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.0f, false);

		if (NavPath && NavPath->PathPoints.Num() > 1) //if onlt just one, we have reach the playerpawn
		{
			//return next point in the path
			return NavPath->PathPoints[1];
		}
	}

	// Failed to find path
	return GetActorLocation();
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent *HealthComp, float Health, float HealthDelta, const class UDamageType *DamageType, class AController *InstigatedBy, AActor *DamageCauser)
{

	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	//Double check the dynamic material is assigned
	if (MatInst)
	{ //The "LastTimeDamageTaken" is derived from the M_trakerbot
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	//UE_LOG(LogTemp, Log, TEXT("Health of %s of %s"), *FString::SanitizeFloat(Health), *GetName());

	//Explode on hitpoints == 0
	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}

void ASTrackerBot::SelfDestruct()
{

	if (bExploded) //make sure will not explode second time
	{
		return;
	}

	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetLocalRole() == ROLE_Authority)
	{
		TArray<AActor *> IgnoreActors;
		IgnoreActors.Add(this);

		//Increase damage based on the power level
		float ActualDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		//Apply damage
		UGameplayStatics::ApplyRadialDamage(this, ActualDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoreActors, this, GetInstigatorController(), true);
		if(DebugTrackerBotDrawing)
		{
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);
		}
		SetLifeSpan(2.0f);
	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor *OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSelfDestruct && !bExploded)
	{
		ASCharacter *PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn && !USHealthComponent::IsFriendly(OtherActor, this))
		{
			//we overlap with a player
			if (GetLocalRole() == ROLE_Authority)
			{
				//Start self destruction sequence
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
			}

			bStartedSelfDestruct = true;

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
		}
	}
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::OnCheckNearbyBots()
{
	const float Radius = 600;

	//Create temporary collision shape for overlapes
	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	//Only find Pawns(eg, player and AI bots)
	FCollisionObjectQueryParams QueryParams;

	//Our Tracker bot's mesh component is set to Physics Body in blueprint (default profile of physics simjlated actors)
	//Search for physics body and pawns
	QueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, QueryParams, CollShape);
	if(DebugTrackerBotDrawing)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.0f);
	}
	int32 NrOfBots = 0;
	//loop over the results using a "range base loop"
	for (FOverlapResult Result : Overlaps)
	{
		//Check if we overlap with other tracker bot(ignoring players and other bot types)
		ASTrackerBot *Bot = Cast<ASTrackerBot>(Result.GetActor());
		//Ignore this tracker bot instance
		if (Bot && Bot != this)
		{
			NrOfBots++;
		}
	}

	const int32 MaxPowerLevel = 4;
	//Clamp between min=0 nad max=4;
	PowerLevel = FMath::Clamp(NrOfBots, 0, MaxPowerLevel);

	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		//Convert to a float between 0 and 1 just like an "Alpha" value of a texture. Now thematerial can be set up without having to know the max power
		//which can be tweaked by gameplay decision (would need 2 places up to date)
		float Alpha = PowerLevel / (float)MaxPowerLevel;
		//Note: (float)MaxPowerLevel converts the int32 to float
		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}
	if(DebugTrackerBotDrawing)
	{
		DrawDebugString(GetWorld(), FVector(0, 0, 0), FString::FromInt(PowerLevel), this, FColor::White, 1.0f, true);
	}
}

void ASTrackerBot::RefreshPath()
{
	NextPathPoint = GetNextPathPoint();
}