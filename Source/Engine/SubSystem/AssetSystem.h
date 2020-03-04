#pragma once
#include "../Interface/IAssetSystem.h"

class InnoAssetSystem : public IAssetSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoAssetSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	bool recordLoadedMeshMaterialPair(const char* fileName, const MeshMaterialPair& pair) override;
	bool findLoadedMeshMaterialPair(const char* fileName, MeshMaterialPair& pair) override;

	bool recordLoadedModel(const char* fileName, const ModelIndex& modelIndex) override;
	bool findLoadedModel(const char* fileName, ModelIndex& modelIndex) override;

	bool recordLoadedTexture(const char* fileName, TextureDataComponent* texture) override;
	bool findLoadedTexture(const char* fileName, TextureDataComponent*& texture) override;

	bool recordLoadedSkeleton(const char* fileName, SkeletonDataComponent* skeleton) override;
	bool findLoadedSkeleton(const char* fileName, SkeletonDataComponent*& skeleton) override;

	bool recordLoadedAnimation(const char* fileName, AnimationDataComponent* animation) override;
	bool findLoadedAnimation(const char* fileName, AnimationDataComponent*& animation) override;

	uint64_t getCurrentMeshMaterialPairOffset() override;
	uint64_t addMeshMaterialPair(const MeshMaterialPair& pair) override;
	const MeshMaterialPair& getMeshMaterialPair(uint64_t index) override;

	ModelIndex addUnitModel(MeshShapeType meshShapeType) override;
	void addUnitCube(MeshDataComponent& meshDataComponent) override;
	void addUnitSphere(MeshDataComponent& meshDataComponent) override;
	void addUnitQuad(MeshDataComponent& meshDataComponent) override;
	void addUnitLine(MeshDataComponent& meshDataComponent) override;
	void addTerrain(MeshDataComponent& meshDataComponent) override;
};
