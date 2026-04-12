#pragma once
#include "../Interface/IService.h"
#include "../Component/GPUBufferComponent.h"

namespace Inno
{
	struct LightDataServiceImpl;
	class LightDataService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(LightDataService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		GPUBufferComponent* GetPointLightBuffer();
		GPUBufferComponent* GetSphereLightBuffer();
		GPUBufferComponent* GetCSMBuffer();
		GPUBufferComponent* GetGIBuffer();

		uint32_t GetPointLightCount();
		uint32_t GetSphereLightCount();

	private:
		LightDataServiceImpl* m_Impl;
	};
}
