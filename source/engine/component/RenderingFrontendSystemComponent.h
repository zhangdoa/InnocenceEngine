#pragma once
#include "../common/InnoType.h"
#include "../system/GPUDataStructureHeader.h"

class RenderingFrontendSystemComponent
{
public:
	~RenderingFrontendSystemComponent() {};

	static RenderingFrontendSystemComponent& get()
	{
		static RenderingFrontendSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	CameraGPUData m_cameraGPUData;
	SunGPUData m_sunGPUData;

	const unsigned int m_maxCSMSplit = 4;
	std::vector<CSMGPUData> m_CSMGPUDataVector;

	const unsigned int m_maxPointLights = 1024;
	std::vector<PointLightGPUData> m_pointLightGPUDataVector;

	const unsigned int m_maxSphereLights = 128;
	std::vector<SphereLightGPUData> m_sphereLightGPUDataVector;

	ThreadSafeQueue<GeometryPassGPUData> m_opaquePassGPUDataQueue;

	ThreadSafeQueue<GeometryPassGPUData> m_transparentPassGPUDataQueue;

	ThreadSafeQueue<BillboardPassGPUData> m_billboardPassGPUDataQueue;

	ThreadSafeQueue<DebuggerPassGPUData> m_debuggerPassGPUDataQueue;

	ThreadSafeQueue<GeometryPassGPUData> m_GIPassGPUDataQueue;

private:
	RenderingFrontendSystemComponent() {};
};
