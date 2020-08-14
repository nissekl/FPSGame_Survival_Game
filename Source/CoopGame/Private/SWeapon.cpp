// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "CoopGame/CoopGame.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(TEXT("COOP.DebugWeapons"), DebugWeaponDrawing, TEXT("Draw Debug Lines for Weapons"), ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;

	RateOfFire = 600; //600 bullets perminute

	BulletSpread = 2.0f;

	SetReplicates(true);//to enable the gun spawned on the sever can be cpoy to client

	//To prevent fire delay between server and client
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}

void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
	TimeBetweenShots = 60/RateOfFire  ;
}

void ASWeapon::Fire()
{
	//if the fire is called in the client will go in to this if and call sever to use the fire() and go through the rest of the code in client  
	if(GetLocalRole() < ROLE_Authority)
	{
		ServerFire();

	}

	//Trace the world. from pawn eyes to crosshair location(which is center screen)
	AActor* MyOwner= GetOwner();
	if(MyOwner)
	{		
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();

		//Bullet Spread
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = EyeLocation + (ShotDirection*10000);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		//Particle "Target" parameter
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if(GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			//Blocking hit! Prepare to give damage;
			AActor* HitActor = Hit.GetActor();
			
			
			//This DetermineSurgace will return the type of the physical surface of the item we hit, the physical surface is set in editing and the dummy 
			//It would return default(no lives got shot) or flesh(shot someone)
			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());
			
			float ActualDamage = BaseDamage;
			if(SurfaceType == SURFACE_FLESHVULNERABLE)
			{
				ActualDamage *=4.0f; 
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

			//Switch to this point when we hit something in front of us
			TracerEndPoint = Hit.ImpactPoint;
		}

		if(DebugWeaponDrawing > 0)
		{
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);
		}
		
		PlayFireEffects(TracerEndPoint);
		
		if(GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;

		}

		//Once we have a successful shot we keep track the time of the world
		LastFireTime = GetWorld()->TimeSeconds;

	}


}

void ASWeapon::PlayFireEffects(FVector TraceEnd)
{
		//Make sure the effect is assigned or selected in blue print
		if(MuzzleEffect)
		{	//Gun fire light
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
		}

		if(TracerEffect)
		{	//Fire smoke line
			FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
			UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
			if(TracerComp)
			{
				TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);

			}


		}

		UGameplayStatics::PlaySound2D(this,GunFiringSound);

		APawn* MyOwner = Cast<APawn>(GetOwner());
		if(MyOwner)
		{
			APlayerController* PC=Cast<APlayerController>(MyOwner->GetController());
			if(PC)
			{
				PC->ClientPlayCameraShake(FireCamShake);
			}
		}

}


void ASWeapon::StartFire()
{		
	/* For example:LastFireTime is 1 second in game time + timebetweenfire(0.1s) - gametime(if we fire int he same frame will also be one). So 1+0.1-1=0.1*/
	/* Use max to make sure the time if greater equal than zero*/
	float FirstDelay = FMath::Max(0.0f, LastFireTime - TimeBetweenShots - GetWorld()->TimeSeconds);
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);	
}


void ASWeapon::ServerFire_Implementation()
{
	Fire();
}

//validate tht codo to see if there's something wrong
bool ASWeapon::ServerFire_Validate()
{
	return true;
}

void ASWeapon::OnRep_HitScanTrace()
{
	//Play cosmetic FX
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

//Let us specify how we want to replicate and what (variable)  do we want to replicate (got this from weird place)
void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//Using condition to ignor the people who shot the fire (becuase the effect is already play on the client once)
	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);//replicate this currentweapon variable to any client connect to us
}


void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint)
{
			UParticleSystem* SelectedEffect = nullptr;
					
			switch(SurfaceType)
			{
			case SURFACE_FLESHDEFAULT:
			case SURFACE_FLESHVULNERABLE:
				SelectedEffect = FleshImpactEffect;
				break;
			default:
				SelectedEffect = DefaultImpactEffect;
				break;
			}

			if(SelectedEffect)
			{	//Blood effect or defaulteffect;
				FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
				FVector ShotDirection = ImpactPoint - MuzzleLocation;
				ShotDirection.Normalize();

				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
			}
			
}

void ASWeapon::SelfDestruction()
{
	SetLifeSpan(1.0f);
}