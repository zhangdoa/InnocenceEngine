#pragma once
#include "../Interface/IService.h"

namespace Inno
{
	class GUIService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(GUIService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool ExecuteCommands();
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}