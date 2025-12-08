// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PlanetFoliage.generated.h"

class APlanet;
class UHierarchicalInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct FFoliageLayerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Foliage")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, Category="Foliage", meta = (ClampMin = "0"))
	int32 InstanceCount = 100000;

	UPROPERTY(EditAnywhere, Category="Foliage", meta = (ClampMin = "1"))
	int32 ClusterCount = 250;

	UPROPERTY(EditAnywhere, Category="Foliage", meta = (ClampMin = "0.0"))
	float ClusterSpread = 0.04f;

	UPROPERTY(EditAnywhere, Category="Foliage", meta = (ClampMin = "-10.0"))
	float SurfaceOffset = 0.0f;

	UPROPERTY(EditAnywhere, Category="Foliage", meta = (ClampMin = "0.0"))
	float MinUniformScale = 0.9f;

	UPROPERTY(EditAnywhere, Category="Foliage", meta = (ClampMin = "0.0"))
	float MaxUniformScale = 1.1f;

	UPROPERTY(EditAnywhere, Category="Foliage|Collision")
	bool bEnableCollision = false;

	UPROPERTY(EditAnywhere, Category="Foliage|Collision")
	bool bAffectNavigation = false;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UPlanetFoliage : public USceneComponent
{
	GENERATED_BODY()

public:
	UPlanetFoliage();

protected:
	virtual void BeginPlay() override;

private:
	void GenerateFoliageInstances();
	void GenerateInstancesForLayer(const FFoliageLayerConfig& Config, UHierarchicalInstancedStaticMeshComponent* Instances);
	void ConfigureInstanceComponent(const FFoliageLayerConfig& Config, UHierarchicalInstancedStaticMeshComponent* Instances) const;
	APlanet* ResolvePlanetActor() const;
	float ResolvePlanetRadius(const APlanet* PlanetActor) const;
	FVector ResolvePlanetCenter(const APlanet* PlanetActor) const;
	float CalculateRandomScale(const FFoliageLayerConfig& Config) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Foliage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Foliage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FlowerInstances;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Foliage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Foliage", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RockInstances;

	UPROPERTY(EditAnywhere, Category="Foliage")
	TObjectPtr<APlanet> PlanetOverride;

	UPROPERTY(EditAnywhere, Category="Foliage", meta=(AllowPrivateAccess="true"))
	FFoliageLayerConfig GrassConfig;

	UPROPERTY(EditAnywhere, Category="Foliage", meta=(AllowPrivateAccess="true"))
	FFoliageLayerConfig Flowerconfig;

	UPROPERTY(EditAnywhere, Category="Foliage", meta=(AllowPrivateAccess="true"))
	FFoliageLayerConfig TreeConfig;

	UPROPERTY(EditAnywhere, Category="Foliage", meta=(AllowPrivateAccess="true"))
	FFoliageLayerConfig RockConfig;

	UPROPERTY(EditAnywhere, Category="Foliage")
	bool bRegenerateOnRegister = true;
};
