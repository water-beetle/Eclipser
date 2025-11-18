// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Planet.generated.h"

class UGravityField;
class UVoxelManager;

UCLASS()
class ECLIPSER_API APlanet : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APlanet();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category="PlanetSettings")
	int32 GetPlanetRadius() const { return PlanetRadius; }

	UPROPERTY(EditAnywhere, Category="PlanetSettings|Voxel", meta=(ClampMin="1", UIMin="1"))
	int32 CellSize = 100;
	UPROPERTY(EditAnywhere, Category="PlanetSettings|Voxel", meta=(ClampMin="1", UIMin="1"))
	int32 CellNum = 10;
	UPROPERTY(EditAnywhere, Category="PlanetSettings|Voxel", meta=(ClampMin="1", UIMin="1"))
	int32 ChunkNum = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PlanetSettings|Voxel", meta=(AllowPrivateAccess=true))
	int32 PlanetDiameter = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PlanetComponent", meta=(AllowPrivateAccess=true))
	TObjectPtr<USceneComponent> PlanetRoot;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlanetComponent")
	TObjectPtr<UVoxelManager> VoxelManager;

	UPROPERTY(EditAnywhere, Category="PlanetComponent")
	TObjectPtr<UGravityField> GravityField;

private:
	void UpdatePlanetConfiguration();
	bool HasValidVoxelConfiguration() const;
	int32 CalculatePlanetDiameter() const;
	void CacheVoxelSettingsFromManager();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PlanetSettings|Voxel", meta=(AllowPrivateAccess=true))
	int PlanetRadius = 1000;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
