#pragma once
#include "IAssetSystem.h"

class InnoAssetSystem : public IAssetSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoAssetSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void addUnitCube(MeshDataComponent& meshDataComponent) override;
	void addUnitSphere(MeshDataComponent& meshDataComponent) override;
	void addUnitQuad(MeshDataComponent& meshDataComponent) override;
	void addUnitLine(MeshDataComponent& meshDataComponent) override;
	void addTerrain(MeshDataComponent& meshDataComponent) override;
};
