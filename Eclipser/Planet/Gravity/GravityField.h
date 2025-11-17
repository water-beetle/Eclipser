// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Character/SpaceCharacter.h"
#include "GravityField.generated.h"


class UBoxComponent;
class UGravityBody;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UGravityField : public USphereComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGravityField();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
public:
	// 중력 방향 계산 함수
	virtual FVector GetGravityDirection(UGravityBody* GravityBody) const PURE_VIRTUAL(UGravityField::GetGravityDirection, return FVector::ZeroVector;);

	// 중력 관련 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	int Priority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
	float GravityScale;

	void SetGravityFieldSize(float Radius);
	void AddCharacterToGravityField();

private:
	// 중력 필드 충돌 처리
	UFUNCTION()
	void OnEnterGravityArea(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
							UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
							bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnExitGravityArea(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
						   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
