// Copyright Epic Games, Inc. All Rights Reserved.

#include "SpaceCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Eclipser.h"
#include "Kismet/GameplayStatics.h"
#include "Character/GravityBody.h"
#include "Planet/Voxel/VoxelChunk.h"

ASpaceCharacter::ASpaceCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraRoot = CreateDefaultSubobject<USceneComponent>("CameraRoot");
	CameraRoot->SetupAttachment(RootComponent);
	
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CameraRoot);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 70.f);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MoveRefRoot = CreateDefaultSubobject<USceneComponent>("MoveRefRoot");
	MoveRefRoot->SetupAttachment(CameraRoot);

	MoveForwardRef = CreateDefaultSubobject<UStaticMeshComponent>("SphereMesh");
	MoveForwardRef->SetupAttachment(MoveRefRoot);
	MoveForwardRef->SetRelativeLocation(FVector(500,0,0));
	MoveForwardRef->SetRelativeScale3D(FVector(.3f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
			TEXT("/Engine/BasicShapes/Sphere.Sphere")
		);
	if (SphereMeshAsset.Succeeded())
	{
		MoveForwardRef->SetStaticMesh(SphereMeshAsset.Object);
	}
	
	GravityBody = CreateDefaultSubobject<UGravityBody>(TEXT("GravityBody"));

}

void ASpaceCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->GravityScale = 0.0f;
	CameraRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	//GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}

void ASpaceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpaceCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpaceCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &ASpaceCharacter::OnLookCompleted);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Canceled,  this, &ASpaceCharacter::OnLookCompleted);

		EnhancedInputComponent->BindAction(DigAction, ETriggerEvent::Started, this, &ASpaceCharacter::OnDigPressed);
		EnhancedInputComponent->BindAction(DigAction, ETriggerEvent::Completed, this, &ASpaceCharacter::OnDigReleased);
		
	}
	else
	{
		UE_LOG(LogEclipser, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASpaceCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector TargetGravityDir = GravityBody->GetGravityDirection().GetSafeNormal();

	CheckIsLanding();

	float Dot = FVector::DotProduct(GravityDir, TargetGravityDir);
	Dot = FMath::Clamp(Dot, -1.f, 1.f);
	float t = (1.f - Dot) * 0.5f;

	const float BaseSpeed = 3.f;
	const float MinFactor = 0.05f;   // 정반대일 때 속도 비율 (0~1)


	float AngleFactor = 1.f - t * (1.f - MinFactor);
	float GravityChangeSpeed = BaseSpeed * AngleFactor;
	
	GravityDir = FMath::VInterpTo(GravityDir, TargetGravityDir, DeltaSeconds, GravityChangeSpeed).GetSafeNormal();
	GetCharacterMovement()->SetGravityDirection(GravityDir);

	UpdateCamera();
}

void ASpaceCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ASpaceCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	LookInput = Value.Get<FVector2D>();

	// route the input
	//DoLook(LookInput.X, LookInput.Y);
}

void ASpaceCharacter::OnLookCompleted(const FInputActionValue& InputActionValue)
{
	LookInput = FVector2D::ZeroVector;
}

void ASpaceCharacter::OnDigPressed()
{
	if (GetWorldTimerManager().IsTimerActive(DigTimerHandle))
		return;
	
	GetWorldTimerManager().SetTimer(
		DigTimerHandle,
		this,
		&ASpaceCharacter::PerformDig,
		DigInterval,
		true // 반복 실행
	);
	
	PerformDig();
}

void ASpaceCharacter::OnDigReleased()
{
	GetWorldTimerManager().ClearTimer(DigTimerHandle);
}

void ASpaceCharacter::PerformDig()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC) || !IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	
	FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    const FVector2D ViewportCenter = ViewportSize / 2.f;

    FVector TraceStart;
    FVector Forward;
    if (!UGameplayStatics::DeprojectScreenToWorld(PC, ViewportCenter, TraceStart, Forward)) return;

	const FVector TraceEnd = TraceStart + Forward * MaxDigDistance;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, DigTraceChannel);
	
	if (UVoxelChunk* HitChunk = Cast<UVoxelChunk>(HitResult.GetComponent()))
	{
		if (UVoxelManager* Manager = HitChunk->GetVoxelManager())
		{
			Manager->Sculpt(HitResult.ImpactPoint, DigRadius);
		}
	}
}

void ASpaceCharacter::UpdateCamera() const
{
	double UpdatedYaw = CameraBoom->GetRelativeRotation().Yaw + LookInput.X;
	double UpdatedPitch = FMath::Clamp(CameraBoom->GetRelativeRotation().Pitch + LookInput.Y, -80, 80); 
	
	CameraBoom->SetRelativeRotation(FRotator(UpdatedPitch, UpdatedYaw, 0));

	FRotator CameraRootRot = FRotationMatrix::MakeFromZX(-GetGravityDirection(), CameraRoot->GetForwardVector()).Rotator();
	CameraRoot->SetWorldLocationAndRotation(GetActorLocation(), CameraRootRot);

	MoveRefRoot->SetRelativeRotation(FRotator(0, CameraBoom->GetRelativeRotation().Yaw, 0));
}

void ASpaceCharacter::CheckIsLanding()
{
	const FVector TraceStart = GetMesh()->GetComponentLocation();
	const FVector TraceEnd = TraceStart - (GetActorUpVector() * 10);
	FHitResult HitResult;

	IsLanding = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, DigTraceChannel);
}

void ASpaceCharacter::DoMove(float Right, float Forward)
{
	// if (GetController() != nullptr)
	// {
	// 	// find out which way is forward
	// 	const FRotator Rotation = GetController()->GetControlRotation();
	// 	const FRotator YawRotation(0, Rotation.Yaw, 0);
	//
	// 	// get forward vector
	// 	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	//
	// 	// get right vector 
	// 	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	//
	// 	// add movement 
	// 	AddMovementInput(ForwardDirection, Forward);
	// 	AddMovementInput(RightDirection, Right);
	// }

	FVector ForwardDirection = FVector::Zero();

	if (GetGravityBody()->IsInGravityField)
	{
		ForwardDirection = (MoveForwardRef->GetComponentLocation() - GetActorLocation()).GetSafeNormal();
	}
	else
	{
		ForwardDirection = FollowCamera->GetForwardVector();
	}
	AddMovementInput(ForwardDirection, Forward);
	AddMovementInput(GetFollowCamera()->GetRightVector(), Right);
}

void ASpaceCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ASpaceCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ASpaceCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
