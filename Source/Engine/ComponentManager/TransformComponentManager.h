#pragma once
#include "ITransformComponentManager.h"

class InnoTransformComponentManager : public ITransformComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTransformComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool PostFrame() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, bool serializable, ObjectLifespan objectLifespan) override;
	void Destroy(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	TransformComponent* Get(std::size_t index) override;
	const std::vector<TransformComponent*>& GetAllComponents() override;
};