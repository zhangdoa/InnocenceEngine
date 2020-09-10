#pragma once
#include "ILightComponentManager.h"

class InnoLightComponentManager : public ILightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, bool serializable, ObjectLifespan objectLifespan) override;
	void Destroy(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<LightComponent*>& GetAllComponents() override;
	const LightComponent* GetSun() override;
	const std::vector<AABB>& GetSunSplitAABB() override;
	const std::vector<Mat4>& GetSunViewMatrices() override;
	const std::vector<Mat4>& GetSunProjectionMatrices() override;
};