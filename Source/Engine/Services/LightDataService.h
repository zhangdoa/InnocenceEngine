#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"

namespace Inno
{
	struct LightDataServiceImpl;
	class LightDataService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightDataService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		GPUBufferComponent* GetPointLightBuffer();
		GPUBufferComponent* GetSphereLightBuffer();
		GPUBufferComponent* GetCSMBuffer();
		GPUBufferComponent* GetGIBuffer();

	private:
		LightDataServiceImpl* m_Impl;
	};
}
