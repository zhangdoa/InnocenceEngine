#pragma once
#include "../Interface/IService.h"

namespace Inno
{
	class LightSimulationService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightSimulationService);

		bool Setup(IServiceConfig*) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
