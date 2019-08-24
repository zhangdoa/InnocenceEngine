#pragma once
#include "IDirectionalLightComponentManager.h"

class InnoDirectionalLightComponentManager : public IDirectionalLightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoDirectionalLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destroy(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<DirectionalLightComponent*>& GetAllComponents() override;
	const std::vector<AABB>& GetSplitAABB() override;
	const std::vector<Mat4>& GetProjectionMatrices() override;
};