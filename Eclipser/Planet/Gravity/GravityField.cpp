// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityField.h"
#include "Character/SpaceCharacter.h"
#include "Character/GravityBody.h"


// Sets default values for this component's properties
UGravityField::UGravityField()
{
	// 이벤트 바인딩
	OnComponentBeginOverlap.AddDynamic(this, &UGravityField::OnEnterGravityArea);
	OnComponentEndOverlap.AddDynamic(this, &UGravityField::OnExitGravityArea);

	GravityScale = 1.0f;
	SetWorldScale3D(FVector(1.f));
	//SetHiddenInGame(false);
}


// Called when the game starts
void UGravityField::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGravityField::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGravityField::SetGravityFieldSize(float Radius)
{
	SetSphereRadius(Radius, true);

	// 이미 캐릭터가 중력장 안에 있으면 overlap 이벤트가 발생되지 않음
	// 중력장 크기 설정 후, 캐릭터가 주위에 있는지 확인
	AddCharacterToGravityField(); 
}

void UGravityField::AddCharacterToGravityField()
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ASpaceCharacter::StaticClass());

	for (AActor* OverlappingActor : OverlappingActors)
	{
		ASpaceCharacter* SpaceCharacter = Cast<ASpaceCharacter>(OverlappingActor);
		if (!SpaceCharacter)
		{
			continue;
		}

		if (UGravityBody* GravityBody = SpaceCharacter-> GetGravityBody())
		{
			GravityBody->AddGravityArea(this);
		}
	}
}

void UGravityField::OnEnterGravityArea(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->IsA(ASpaceCharacter::StaticClass()))
	{
		ASpaceCharacter* SpaceCharacter = Cast<ASpaceCharacter>(OtherActor);
		if (SpaceCharacter)
		{
			SpaceCharacter-> GetGravityBody()->AddGravityArea(this);
		}
	}
}

void UGravityField::OnExitGravityArea(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor->IsA(ASpaceCharacter::StaticClass()))
	{
		ASpaceCharacter* SpaceCharacter = Cast<ASpaceCharacter>(OtherActor);
		if (SpaceCharacter)
		{
			SpaceCharacter-> GetGravityBody()->RemoveGravityArea(this);
		}
	}
}

