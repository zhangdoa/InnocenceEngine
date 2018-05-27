#pragma once
#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IGameSystem.h"

#include "component/MeshDataComponent.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

class MeshDataSystem : public IRenderingSystem
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
