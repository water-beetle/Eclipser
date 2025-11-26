// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PlanetGrass.generated.h"

class APlanet;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UPlanetGrass : public USceneComponent
{
	GENERATED_BODY()

public:
	UPlanetGrass();
	
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Grass")
	void RegenerateGrassInstances();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

private:
	void GenerateGrassInstances();
	APlanet* ResolvePlanetActor() const;
	float ResolvePlanetRadius(const APlanet* PlanetActor) const;
	FVector ResolvePlanetCenter(const APlanet* PlanetActor) const;
	float CalculateRandomScale() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grass", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassInstances;

	UPROPERTY(EditAnywhere, Category="Grass")
	TObjectPtr<APlanet> PlanetOverride;

	UPROPERTY(EditAnywhere, Category="Grass", meta = (ClampMin = "0"))
	int32 InstanceCount = 100000;

	UPROPERTY(EditAnywhere, Category="Grass", meta = (ClampMin = "1"))
	int32 ClusterCount = 250;

	UPROPERTY(EditAnywhere, Category="Grass", meta = (ClampMin = "0.0"))
	float ClusterSpread = 0.04f;

	UPROPERTY(EditAnywhere, Category="Grass", meta = (ClampMin = "0.0"))
	float SurfaceOffset = 0.0f;

	UPROPERTY(EditAnywhere, Category="Grass", meta = (ClampMin = "0.0"))
	float MinUniformScale = 0.9f;

	UPROPERTY(EditAnywhere, Category="Grass", meta = (ClampMin = "0.0"))
	float MaxUniformScale = 1.1f;

	UPROPERTY(EditAnywhere, Category="Grass")
	bool bRegenerateOnRegister = true;
};
