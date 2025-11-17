// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SpaceCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UGravityBody;
struct FInputActionValue;


/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class ASpaceCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* CameraRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* MoveRefRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MoveForwardRef;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DigAction;

public:

	/** Constructor */
	ASpaceCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	void OnLookCompleted(const FInputActionValue& InputActionValue);


public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);
	
	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:

	void OnDigPressed();
	void OnDigReleased();
	void PerformDig();
	
	FTimerHandle DigTimerHandle;

	UPROPERTY(EditAnywhere, Category="Dig")
	float DigInterval = 0.3f;

	UPROPERTY(EditAnywhere, Category="Dig")
	TEnumAsByte<ECollisionChannel> DigTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category="Dig")
	float MaxDigDistance = 300.f;  // // 플레이어와 파는 지점 간 최대 거리

	UPROPERTY(EditAnywhere, Category="Dig")
	float DigRadius = 50.0f; // 땅 파는 반경

public:
	/* Gravity */
	UPROPERTY()
	UGravityBody* GravityBody;
	
	class UGravityBody* GetGravityBody() const {return GravityBody;}

private:
	/* Gravity */
	FVector GravityDir;
	FVector2D LookInput = FVector2D::ZeroVector;
	
	void UpdateCamera() const; // 중력 방향에 따라 Camera의 UpVector를 매초 Update해주는 함수
};

