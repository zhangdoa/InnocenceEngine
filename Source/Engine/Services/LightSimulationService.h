#pragma once
#include "../Interface/ISystem.h"

namespace Inno
{
	class LightSimulationService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightSimulationService);

		bool Setup(ISystemConfig*) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
