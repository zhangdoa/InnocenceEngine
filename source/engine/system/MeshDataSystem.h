#pragma once
#include "interface/ISystem.h"
#include "component/AssetSystemSingletonComponent.h"

class MeshDataSystem : public ISystem
{
public:
	MeshDataSystem() {};
	~MeshDataSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void addUnitCube(MeshDataComponent& meshDataComponent);
	void addUnitSphere(MeshDataComponent& meshDataComponent);
	void addUnitQuad(MeshDataComponent& meshDataComponent);
	void addUnitLine(MeshDataComponent& meshDataComponent);

	vec4 findMaxVertex(const MeshDataComponent& meshDataComponent) const;
	vec4 findMinVertex(const MeshDataComponent& meshDataComponent) const;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};
