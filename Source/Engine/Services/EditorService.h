#pragma once

#include "../Interface/IService.h"

namespace Inno
{
	class EditorService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(EditorService);

		bool Setup(IServiceConfig* config) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	};
}
