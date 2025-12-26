// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Planet/Voxel/Defines/VoxelStructs.h"
#include "PlanetFoliage.generated.h"

class APlanet;
class UHierarchicalInstancedStaticMeshComponent;
class UVoxelManager;

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

	// false일 경우 mesh의 boundbox와 겹치는 foliage 삭제,
	// tree같은 경우는 bound가 커서 거리 계산으로 삭제해야함
	UPROPERTY(EditAnywhere, Category="Foliage|Collision")
	bool bRemoveByDistance = false;	
	
	UPROPERTY(EditAnywhere, Category="Foliage|Collision")
	bool bEnableCollision = false;

	UPROPERTY(EditAnywhere, Category="Foliage|Collision")
	bool bAffectNavigation = false;
};

USTRUCT()
struct FChunkFoliageComponents
{
	GENERATED_BODY()

	UPROPERTY()
	FVector ChunkCenterWorld;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Grass;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Flower;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Tree;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rock;
};

struct FPlanetSurfaceQueryData
{
	FVector PlanetCenter = FVector::ZeroVector;
	int32 PlanetRadius = 0;
	FPlanetNoiseSettings NoiseSettings;
	bool bHasNoiseSettings = false;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ECLIPSER_API UPlanetFoliage : public USceneComponent
{
	GENERATED_BODY()

public:
	UPlanetFoliage();

	void RemoveInstancesWithinRadius(const FVector& WorldCenter, float Radius);
protected:
	virtual void BeginPlay() override;

private:
	void GenerateFoliageInstances();
	void RemoveInstancesFromComponent(UHierarchicalInstancedStaticMeshComponent* Instances, const FVector& WorldCenter,
										  float Radius, bool bRemoveByDistance) const;
	TArray<FTransform> GenerateTransformsForLayer(const FFoliageLayerConfig& Config, const FVector& ChunkRelativeCenter,
		float ChunkHalfSize, const FPlanetSurfaceQueryData& SurfaceData) const;
	void ConfigureInstanceComponent(const FFoliageLayerConfig& Config, UHierarchicalInstancedStaticMeshComponent* Instances) const;
	APlanet* ResolvePlanetActor() const;
	const UVoxelManager* ResolveVoxelManager() const;
	int32 ResolveChunkNum(const APlanet* PlanetActor, const UVoxelManager* VoxelManager) const;
	int32 ResolveCellSize(const APlanet* PlanetActor, const UVoxelManager* VoxelManager) const;
	int32 ResolveCellNum(const APlanet* PlanetActor, const UVoxelManager* VoxelManager) const;
	float ResolvePlanetRadius(const APlanet* PlanetActor) const;
	FVector ResolvePlanetCenter(const APlanet* PlanetActor) const;
	float CalculateRandomScale(const FFoliageLayerConfig& Config) const;
	
	void ClearChunkFoliage();
	UHierarchicalInstancedStaticMeshComponent* CreateChunkComponent(const FString& BaseName);
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

	UPROPERTY(Transient)
	TMap<FIntVector, FChunkFoliageComponents> ChunkFoliageMap;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> SpawnedChunkComponents;

	float CachedChunkHalfSize = 0.0f;
};
