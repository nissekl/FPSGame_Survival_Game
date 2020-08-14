// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectileWeapon.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ASProjectileWeapon::ASProjectileWeapon(): ASWeapon()
{
    SetReplicates(true);
}

void ASProjectileWeapon::Fire()
{
    
    if(GetLocalRole() < ROLE_Authority)
	{
		ServerFire();

	}
    AActor* MyOwner= GetOwner();
    //ProjectileClass is for checking the projectile is set
	if(MyOwner && ProjectileClass)
	{		
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

        FVector ShotDirection = EyeRotation.Vector();
        FVector TraceEnd = EyeLocation + (ShotDirection*10000);

        FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
       

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if(GetLocalRole() == ROLE_Authority)
		{
            UGameplayStatics::PlaySound2D(this,GunFiringSound);
            GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);
        }
    }
}