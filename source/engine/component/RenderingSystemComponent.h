#pragma once
#include "../common/InnoType.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"
#include<atomic>

struct RenderDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	MeshDataComponent* MDC;
	MaterialDataComponent* Material;
	VisiblilityType visiblilityType;
};

enum class RenderPassType { OpaquePass, TransparentPass, TerrainPass, LightPass };

class RenderingSystemComponent
{
public:
	~RenderingSystemComponent() {};
	
	static RenderingSystemComponent& get()
	{
		static RenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::atomic<bool> m_canRender;
	bool m_shouldUpdateEnvironmentMap = true;
	bool m_isTAAPingPass = true;
	std::vector<vec2> HaltonSampler;
	int currentHaltonStep = 0;
	int m_MSAAdepth = 4;
	bool m_useTAA = false;
	bool m_useBloom = false;
	bool m_drawTerrain = false;
	bool m_drawSky = false;
	bool m_drawOverlapWireframe = false;
	std::function<void(RenderPassType)> f_reloadShader;
	std::vector<RenderDataPack> m_renderDataPack;

private:
	RenderingSystemComponent() {};
};
