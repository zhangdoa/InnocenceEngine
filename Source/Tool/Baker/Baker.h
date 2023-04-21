#pragma once
#include "Common.h"
#include "../../Engine/Interface/IRenderingClient.h"

using namespace Inno;
namespace Inno
{
	namespace Baker
	{
		void Setup();
		void BakeProbeCache(const char* sceneName);
		void BakeBrickCache(const char* surfelCacheFileName);
		void BakeBrick(const char* brickCacheFileName);
		void BakeBrickFactor(const char* brickFileName);

		class Config
		{
			INNO_CLASS_SINGLETON(Config)

			std::string parseFileName(const char* fileName);

			std::string m_exportFileName;

			const uint32_t m_probeMapResolution = 1024;
			const float m_probeHeightOffset = 6.0f;
			const uint32_t m_probeInterval = 64;
			const uint32_t m_captureResolution = 32;
			const uint32_t m_surfelSampleCountPerFace = 32;
			const TVec4<double> m_brickSize = TVec4<double>(4.0, 4.0, 4.0, 0.0);
			const TVec4<double> m_halfBrickSize = m_brickSize / 2.0;

			uint32_t m_staticMeshDrawCallCount = 0;
			std::vector<DrawCallInfo> m_staticMeshDrawCallInfo;
            std::vector<PerObjectConstantBuffer> m_staticMeshPerObjectConstantBuffer;
            std::vector<MaterialConstantBuffer> m_staticMeshMaterialConstantBuffer;
		};
	}

	class BakerRenderingClient: public IRenderingClient
	{
		// Inherited via IRenderingClient
		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Render(IRenderingConfig* renderingConfig = nullptr) override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;
	};
}