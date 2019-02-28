#pragma once
#include "../common/InnoType.h"
#include "VisibleComponent.h"
#include "MeshDataComponent.h"
#include "MaterialDataComponent.h"

#include "../common/InnoConcurrency.h"

struct RenderDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	MeshDataComponent* MDC;
	MaterialDataComponent* material;
	VisiblilityType visiblilityType;
};

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

	std::atomic<bool> m_allowRender = false;
	std::atomic<bool> m_isRendering = false;
	std::atomic<bool> m_isRenderDataPackValid = false;

	bool m_isTAAPingPass = true;

	mat4 m_CamProjOriginal;
	mat4 m_CamProjJittered;
	mat4 m_CamRot;
	mat4 m_CamTrans;
	mat4 m_CamRot_prev;
	mat4 m_CamTrans_prev;
	vec4 m_CamGlobalPos;

	vec4 m_sunDir;
	vec4 m_sunLuminance;
	mat4 m_sunRot;

	std::vector<mat4> m_CSMProjs;
	std::vector<mat4> m_CSMViews;
	std::vector<vec4> m_CSMSplitCorners;

	std::vector<vec2> HaltonSampler;
	int currentHaltonStep = 0;

	int m_MSAAdepth = 4;
	bool m_useTAA = false;
	bool m_useBloom = false;
	bool m_drawTerrain = false;
	bool m_drawSky = false;
	bool m_drawOverlapWireframe = false;

	std::function<void(RenderPassType)> f_reloadShader;
	std::function<void()> f_captureEnvironment;	
	ThreadSafeVector<RenderDataPack> m_renderDataPack;

	VisibleComponent* m_selectedVisibleComponent;
	std::vector<Sphere> m_debugSpheres;
	std::vector<Plane> m_debugPlanes;
private:
	RenderingSystemComponent() {};
};