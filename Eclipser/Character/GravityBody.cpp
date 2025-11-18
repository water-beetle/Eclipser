// Fill out your copyright notice in the Description page of Project Settings.

#include "GravityBody.h"
#include "Planet/Gravity/GravityField.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values for this component's properties
UGravityBody::UGravityBody()
	: CurrentGravityField(nullptr)
	,LastGravityDirection(FVector::DownVector)

{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGravityBody::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGravityBody::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UGravityBody::AddGravityArea(UGravityField* GravityField)
{
	// 새로운 중력 영역을 배열에 추가
	GravityFields.Add(GravityField);

	// 우선순위에 따라 정렬 (Priority 값이 클수록 높은 우선순위)
	GravityFields.Sort([](const UGravityField& A, const UGravityField& B)
	{
		return A.Priority < B.Priority;
	});

	if (ACharacter* GravityCharacter = Cast<ACharacter>(GetOwner()))
	{
		// 현재 적용 중인 중력 영역이 없거나,
		// 새로 들어온 중력 영역의 Priority가 기존 영역보다 높은 경우에만 업데이트
		if (!CurrentGravityField || GravityField->Priority > CurrentGravityField->Priority)
		{
			CurrentGravityField = GravityField;
			IsInGravityField = true;
			GravityCharacter->GetCharacterMovement()->GravityScale = CurrentGravityField->GravityScale;
			GravityCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
	}
}

void UGravityBody::RemoveGravityArea(UGravityField* GravityField)
{
	// 배열에서 해당 중력 영역을 제거
	GravityFields.Remove(GravityField);

	// 우선순위 정렬 (제거 후에도 정렬)
	GravityFields.Sort([](const UGravityField& A, const UGravityField& B)
	{
		return A.Priority < B.Priority;
	});

	if (ACharacter* GravityCharacter = Cast<ACharacter>(GetOwner()))
	{
		// 배열에 남아있는 중력 영역이 없으면 중력 효과를 제거
		if (GravityFields.Num() == 0)
		{
			GravityCharacter->GetCharacterMovement()->GravityScale = 0.0f;
			GravityCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			CurrentGravityField = nullptr;
			IsInGravityField = false;
		}
		else
		{
			// 만약 제거한 중력 영역이 현재 적용 중인 영역이었다면,
			// 가장 우선순위가 높은(정렬된 배열의 마지막) 중력 영역으로 갱신한다.
			if (CurrentGravityField == GravityField)
			{
				CurrentGravityField = GravityFields.Last();
				GravityCharacter->GetCharacterMovement()->GravityScale = CurrentGravityField->GravityScale;
			}
			// 제거한 영역이 현재 영역이 아니라면 그대로 유지한다.
		}
	}
}

FVector UGravityBody::GetGravityDirection()
{
	// GravityFields가 비어있거나 CurrentGravityField가 없으면 기본 방향 반환
	if (GravityFields.Num() == 0 || !CurrentGravityField)
	{
		return LastGravityDirection;
	}
	
	// 가장 우선순위가 높은 중력 영역의 방향 반환
	const FVector CurrentDirection = CurrentGravityField->GetGravityDirection(this).GetSafeNormal();
	if (!CurrentDirection.IsNearlyZero())
	{
		LastGravityDirection = CurrentDirection;
	}
	return LastGravityDirection;
}

