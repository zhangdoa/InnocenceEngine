#pragma once
#include "../common/InnoType.h"
#include "../common/InnoContainer.h"
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

	const unsigned int m_maxMeshes = 16384;
	const unsigned int m_maxMaterials = 32768;
	const unsigned int m_maxTextures = 32768;

	ThreadSafeQueue<GeometryPassGPUData> m_opaquePassGPUDataQueue;

	ThreadSafeQueue<GeometryPassGPUData> m_transparentPassGPUDataQueue;

	ThreadSafeQueue<BillboardPassGPUData> m_billboardPassGPUDataQueue;

	ThreadSafeQueue<DebuggerPassGPUData> m_debuggerPassGPUDataQueue;

	ThreadSafeQueue<GeometryPassGPUData> m_GIPassGPUDataQueue;

	SkyGPUData m_skyGPUData;
	DispatchParamsGPUData m_dispatchParamsGPUData;

private:
	RenderingFrontendSystemComponent() {};
};
