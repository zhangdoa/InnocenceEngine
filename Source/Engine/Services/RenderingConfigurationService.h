#pragma once

#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct RenderingConfig
	{
		bool VSync = false;
		int32_t MSAAdepth = 4;
		bool useCSM = false;
		int32_t shadowMapResolution = 2048;
		bool useMotionBlur = false;
		bool useTAA = false;
		bool useBloom = false;
		bool drawTerrain = false;
		bool drawSky = false;
		bool drawDebugObject = false;
		bool CSMFitToScene = false;
		bool CSMAdjustDrawDistance = false;
		bool CSMAdjustSidePlane = false;
	};

	struct RenderingCapability
	{
		uint32_t maxCSMSplits;
		uint32_t maxPointLights;
		uint32_t maxSphereLights;
		uint32_t maxMeshes;
		uint32_t maxTextures;
		uint32_t maxMaterials;
		uint32_t maxBuffers;
	};
	
	class RenderingConfigurationService
	{
	public:
		RenderingConfigurationService();

		TVec2<uint32_t> GetScreenResolution();
		void SetScreenResolution(TVec2<uint32_t> screenResolution);

		RenderingConfig GetRenderingConfig();
		void SetRenderingConfig(RenderingConfig renderingConfig);

		RenderingCapability GetRenderingCapability();
        
		RenderPassDesc GetDefaultRenderPassDesc();

    private:
        TVec2<uint32_t> m_screenResolution = TVec2<uint32_t>(1280, 720);
		std::string m_windowName;
		bool m_fullScreen = false;
        
		RenderingConfig m_renderingConfig;
        RenderingCapability m_renderingCapability;
        RenderPassDesc m_DefaultRenderPassDesc;
	};
}