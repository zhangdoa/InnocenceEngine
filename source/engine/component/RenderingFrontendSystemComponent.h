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

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
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

	unsigned int m_opaquePassDrawcallCount = 0;
	std::vector<OpaquePassGPUData> m_opaquePassGPUDatas;
	std::vector<MeshGPUData> m_opaquePassMeshGPUDatas;
	std::vector<MaterialGPUData> m_opaquePassMaterialGPUDatas;

	unsigned int m_transparentPassDrawcallCount = 0;
	std::vector<TransparentPassGPUData> m_transparentPassGPUDatas;
	std::vector<MeshGPUData> m_transparentPassMeshGPUDatas;
	std::vector<MaterialGPUData> m_transparentPassMaterialGPUDatas;

	ThreadSafeQueue<BillboardPassGPUData> m_billboardPassGPUDataQueue;

	ThreadSafeQueue<DebuggerPassGPUData> m_debuggerPassGPUDataQueue;

	SkyGPUData m_skyGPUData;
	DispatchParamsGPUData m_dispatchParamsGPUData;

	unsigned int m_GIPassDrawcallCount = 0;
	std::vector<OpaquePassGPUData> m_GIPassGPUDatas;
	std::vector<MeshGPUData> m_GIPassMeshGPUDatas;
	std::vector<MaterialGPUData> m_GIPassMaterialGPUDatas;

private:
	RenderingFrontendSystemComponent() {};
};
