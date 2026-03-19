#pragma once
#include "../Interface/ISystem.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"

#include "../Common/GPUDataStructure.h"

#include "AnimationService.h"

namespace Inno
{
	enum class GPUBufferUsageType
	{
		Billboard
	};

	struct RenderingContextServiceImpl;
	class RenderingContextService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(RenderingContextService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		GPUBufferComponent* GetGPUBufferComponent(GPUBufferUsageType usageType);

		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
		const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();

	private:
		RenderingContextServiceImpl* m_Impl;
	};
}