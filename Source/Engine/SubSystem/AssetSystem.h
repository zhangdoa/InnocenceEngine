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

	bool addModel(const char* fileName, const ModelIndex& modelIndex) override;
	bool getModel(const char* fileName, ModelIndex& modelIndex) override;

	bool addTexture(const char* fileName, TextureDataComponent* texture) override;
	bool getTexture(const char* fileName, TextureDataComponent*& texture) override;

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
