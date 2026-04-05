#pragma once
#include "../Interface/IService.h"
#include "../Common/NamedObjectPool.h"
#include "../Component/ShaderProgramComponent.h"

namespace Inno
{
	class ShaderProgramResourceService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ShaderProgramResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override;
		ObjectStatus GetStatus() override { return m_ObjectStatus; }

		ShaderProgramComponent* Add(const char* name);
		virtual bool Delete(ShaderProgramComponent* ptr);

		void Initialize(ShaderProgramComponent* shaderProgram);

	protected:
		virtual bool InitializeImpl(ShaderProgramComponent* shaderProgram) { return false; }

		NamedObjectPool<ShaderProgramComponent> m_Pool;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
