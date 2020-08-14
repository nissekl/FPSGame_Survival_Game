// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplosiveBarrel.h"
#include "kismet/GameplayStatics.h"
#include "Components/SHealthComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASExplosiveBarrel::ASExplosiveBarrel()
{
	BarrelHealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("BarrelHealthComp"));
	BarrelHealthComp->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::OnHealthChanged);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp -> SetSimulatePhysics(true);
	//Set to physics body to let radial component affect us(eg. when a nearby barrel explodes)
	MeshComp -> SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 250;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bAutoActivate = false;//Prevent component from ticking, and only use FireImpulse() instead
	RadialForceComp->bIgnoreOwningActor = true;//ignore itself, only apply force to others

	ExplosionImpulse = 400;

	SetReplicates(true);//to enable the barrel spawned on the sever can be cpoy to client
	SetReplicateMovement(true);
}







void ASExplosiveBarrel::OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if(Health <= 0.0f && !bExploded)
	{
		//Die!
		bExploded = true;
		OnRep_Exploded();//if we don't use this on OnRep, it will only replicate the explosion effect to on client and the sever still stay color unchanged
		//Boosted barrel upward	
		FVector BoostIntensity = FVector::UpVector*ExplosionImpulse;
		MeshComp->AddImpulse(BoostIntensity, NAME_None, true);
		


		//Blast Away nearby actor
		RadialForceComp->FireImpulse();


	}
}

void ASExplosiveBarrel::OnRep_Exploded()
{
		//PlayExplosioneffect
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

		//Change materil to black
		MeshComp->SetMaterial(0, ExplodedMaterial);
}

//Let us specify how we want to replicate and what (variable)  do we want to replicate (got this from weird place)
void ASExplosiveBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASExplosiveBarrel, bExploded);//replicate this currentweapon variable to any client connect to us
}
