// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "SGameMode.h"

// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{

	DefaultHealth = 100;
	bIsDead = false;

	TeamNum = 255;

	
}

// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	//Only hook if we are the server, cuz it's not actor so use GetOwnerRole
	if (GetOwnerRole() == ROLE_Authority)
	{
		AActor *MyOwner = GetOwner();
		if (MyOwner)
		{ //Subscribe ourselves at the begining, so we can automatically apply
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}

	Health = DefaultHealth;
}

void USHealthComponent::HandleTakeAnyDamage(AActor *DamagedActor, float Damage, const class UDamageType *DamageType, class AController *InstigatedBy, AActor *DamageCauser)
{
	if (Damage <= 0.0f || bIsDead)
	{
		return;
	}
	
	/*DamageCauser!= DamagedActor  is for trackervot self-explostion*/
	if(DamageCauser!= DamagedActor && IsFriendly(DamagedActor, DamageCauser))
	{
		return;
	}

	//Update health clamped
	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);

	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s"), *FString::SanitizeFloat(Health));

	bIsDead = Health<=0.0f;

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);
	if (bIsDead)
	{
		ASGameMode *GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

//Let us specify how we want to replicate and what (variable)  do we want to replicate (got this from weird place)
void USHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USHealthComponent, Health); //replicate this currentweapon variable to any client connect to us
}

void USHealthComponent::OnRep_Health(float OldHealth)
{
	float Damage = Health - OldHealth;

	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void USHealthComponent::Heal(float HealAmount)
{
	if (HealAmount <= 0.0f || Health <= 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + HealAmount, 0.0f, DefaultHealth);

	//santizefloat is remove all the zero
	UE_LOG(LogTemp, Log, TEXT("Health Changed: %s (+%s)"), *FString::SanitizeFloat(Health), *FString::SanitizeFloat(HealAmount));

	OnHealthChanged.Broadcast(this, Health, -HealAmount, nullptr, nullptr, nullptr);
}

float USHealthComponent::GetHealth() const
{
	return Health;
}


bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB)
{
	if(ActorA == nullptr || ActorB == nullptr)
	{
		//Assume friendly
		return true;
	}
	USHealthComponent* HealthCompA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	USHealthComponent* HealthCompB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if(HealthCompA == nullptr || HealthCompB == nullptr)
	{
		//Assume friendly
		return true;
	}

	return HealthCompA->TeamNum == HealthCompB->TeamNum;

}
