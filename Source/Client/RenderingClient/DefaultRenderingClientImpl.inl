#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "TiledFrustumGenerationPass.h"
#include "LightCullingPass.h"
#include "GIDataLoader.h"
#include "GIResolvePass.h"
#include "SurfelGITestPass.h"
#include "LuminanceHistogramPass.h"
#include "LuminanceAveragePass.h"
#include "SunShadowGeometryProcessPass.h"
#include "SunShadowBlurOddPass.h"
#include "SunShadowBlurEvenPass.h"
#include "OpaquePass.h"
#include "AnimationPass.h"
#include "SSAOPass.h"
#include "LightPass.h"
#include "SkyPass.h"
#include "PreTAAPass.h"
#include "TransparentGeometryProcessPass.h"
#include "TransparentBlendPass.h"
#include "VolumetricPass.h"
#include "VXGIRenderer.h"
#include "TAAPass.h"
#include "PostTAAPass.h"
#include "MotionBlurPass.h"
#include "BillboardPass.h"
#include "DebugPass.h"
#include "FinalBlendPass.h"

#include "BSDFTestPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

namespace Inno
{
	class DefaultRenderingClientImpl: public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DefaultRenderingClientImpl);

		// Inherited via IRenderingClient
		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Render(IRenderingConfig* renderingConfig = nullptr) override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		std::function<void()> f_showLightHeatmap;
		std::function<void()> f_showProbe;
		std::function<void()> f_showVoxel;
		std::function<void()> f_showTransparent;
		std::function<void()> f_showVolumetric;
		std::function<void()> f_saveScreenCapture;

		std::function<void()> f_SetupJob;
		std::function<void()> f_InitializeJob;
		std::function<void()> f_RenderJob;
		std::function<void()> f_TerminateJob;

		bool m_showLightHeatmap = false;
		bool m_showProbe = false;
		bool m_showVoxel = false;
		bool m_showTransparent = false;
		bool m_showVolumetric = false;
		bool m_saveScreenCapture = false;
		bool m_drawBRDFTest = false;

		VXGIRendererSystemConfig m_VXGIRendererSystemConfig = {};
		bool m_isPassOdd = false;

	private:
		ObjectStatus m_ObjectStatus;
	};

	bool DefaultRenderingClientImpl::Setup(ISystemConfig* systemConfig)
	{
		f_showLightHeatmap = [&]() { m_showLightHeatmap = !m_showLightHeatmap; };
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_H, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showLightHeatmap });

		f_showProbe = [&]() { m_showProbe = !m_showProbe; };
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_G, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showProbe });

		f_showVoxel = [&]() { m_showVoxel = !m_showVoxel; };
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_V, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVoxel });

		f_showTransparent = [&]() { m_showTransparent = !m_showTransparent; };
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_T, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showTransparent });

		f_showVolumetric = [&]() { m_showVolumetric = !m_showVolumetric; };
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_J, true }, ButtonEvent{ EventLifeTime::OneShot, &f_showVolumetric });

		f_saveScreenCapture = [&]() { m_saveScreenCapture = !m_saveScreenCapture; };
		g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_C, true }, ButtonEvent{ EventLifeTime::OneShot, &f_saveScreenCapture });

		f_SetupJob = [&]() {
			DefaultGPUBuffers::Setup();
			GIDataLoader::Setup();
			BRDFLUTPass::Get().Setup();
			BRDFLUTMSPass::Get().Setup();

			TiledFrustumGenerationPass::Get().Setup();
			LightCullingPass::Get().Setup();
			GIResolvePass::Setup();
			SurfelGITestPass::Get().Setup();
			LuminanceHistogramPass::Get().Setup();
			LuminanceAveragePass::Get().Setup();

			SunShadowGeometryProcessPass::Get().Setup();
			SunShadowBlurOddPass::Get().Setup();
			SunShadowBlurEvenPass::Get().Setup();
			VXGIRenderer::Get().Setup(&m_VXGIRendererSystemConfig);
			OpaquePass::Get().Setup();
			AnimationPass::Get().Setup();

			SSAOPass::Get().Setup();
			LightPass::Get().Setup();
			SkyPass::Get().Setup();
			PreTAAPass::Get().Setup();
			TransparentGeometryProcessPass::Get().Setup();
			TransparentBlendPass::Get().Setup();
			VolumetricPass::Setup();
			TAAPass::Get().Setup();
			PostTAAPass::Get().Setup();
			MotionBlurPass::Get().Setup();
			BillboardPass::Get().Setup();
			DebugPass::Get().Setup();
			FinalBlendPass::Get().Setup();

			BSDFTestPass::Get().Setup();
		};

		f_InitializeJob = [&]() {
			auto l_renderingServer = g_Engine->getRenderingServer();

			DefaultGPUBuffers::Initialize();
			GIDataLoader::Initialize();
			BRDFLUTPass::Get().Initialize();
			BRDFLUTMSPass::Get().Initialize();

			BRDFLUTPass::Get().PrepareCommandList();
			BRDFLUTMSPass::Get().PrepareCommandList();
			l_renderingServer->ExecuteCommandList(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->WaitCommandQueue(BRDFLUTPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->WaitFence(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitFence(BRDFLUTMSPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

			TiledFrustumGenerationPass::Get().Initialize();
			LightCullingPass::Get().Initialize();
			GIResolvePass::Initialize();
			SurfelGITestPass::Get().Initialize();
			LuminanceHistogramPass::Get().Initialize();
			LuminanceAveragePass::Get().Initialize();

			SunShadowGeometryProcessPass::Get().Initialize();
			SunShadowBlurOddPass::Get().Initialize();
			SunShadowBlurEvenPass::Get().Initialize();
			VXGIRenderer::Get().Initialize();
			OpaquePass::Get().Initialize();
			AnimationPass::Get().Initialize();

			SSAOPass::Get().Initialize();
			LightPass::Get().Initialize();
			SkyPass::Get().Initialize();
			PreTAAPass::Get().Initialize();
			TransparentGeometryProcessPass::Get().Initialize();
			TransparentBlendPass::Get().Initialize();
			VolumetricPass::Initialize();
			TAAPass::Get().Initialize();
			PostTAAPass::Get().Initialize();
			MotionBlurPass::Get().Initialize();
			BillboardPass::Get().Initialize();
			DebugPass::Get().Initialize();
			FinalBlendPass::Get().Initialize();

			BSDFTestPass::Get().Initialize();

			auto f_getUserPipelineOutputFunc = []()
			{
				return FinalBlendPass::Get().GetResult();
			};

			l_renderingServer->SetUserPipelineOutput(std::move(f_getUserPipelineOutputFunc));
		};

		f_RenderJob = [&]() {
			auto l_renderingConfig = g_Engine->getRenderingFrontend()->GetRenderingConfig();
			auto l_renderingServer = g_Engine->getRenderingServer();
			GPUResourceComponent* l_canvas;
			RenderPassComponent* l_canvasOwner;

			DefaultGPUBuffers::Upload();

			TiledFrustumGenerationPass::Get().PrepareCommandList();
			LightCullingPass::Get().PrepareCommandList();

			l_renderingServer->ExecuteCommandList(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->WaitCommandQueue(TiledFrustumGenerationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

			SunShadowGeometryProcessPass::Get().PrepareCommandList();

			// The blurring would cause precision errors and then flickering when camera rotates
			// SunShadowBlurOddPass::Get().PrepareCommandList();
			// SunShadowBlurEvenPass::Get().PrepareCommandList();

			l_renderingServer->ExecuteCommandList(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);

			//l_renderingServer->WaitCommandQueue(SunShadowGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			// l_renderingServer->ExecuteCommandList(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			// l_renderingServer->WaitCommandQueue(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			// l_renderingServer->ExecuteCommandList(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

			// l_renderingServer->WaitCommandQueue(SunShadowBlurOddPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			// l_renderingServer->ExecuteCommandList(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			// l_renderingServer->WaitCommandQueue(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			// l_renderingServer->ExecuteCommandList(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

			m_VXGIRendererSystemConfig.m_VXGIRenderingConfig.m_visualize = m_showVoxel;
			VXGIRenderer::Get().Render(&m_VXGIRendererSystemConfig.m_VXGIRenderingConfig);

			if (m_drawBRDFTest)
			{
				BSDFTestPass::Get().PrepareCommandList();
				l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				l_canvas = BSDFTestPass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture;
				l_canvasOwner = BSDFTestPass::Get().GetRenderPassComp();
			}
			else if (m_showLightHeatmap)
			{
				l_canvas = LightCullingPass::Get().GetHeatMap();
				l_canvasOwner = LightCullingPass::Get().GetRenderPassComp();
			}
			else if (m_showProbe)
			{
				SurfelGITestPass::Get().PrepareCommandList();
				l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(BSDFTestPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				l_canvas = SurfelGITestPass::Get().GetRenderPassComp()->m_RenderTargets[0].m_Texture;
				l_canvasOwner = SurfelGITestPass::Get().GetRenderPassComp();
			}
			else if (m_showVolumetric)
			{
				VolumetricPass::Render(true);
				l_canvas = VolumetricPass::GetVisualizationResult();
			}
			else
			{
				//GIResolvePass::PrepareCommandList();

				OpaquePass::Get().PrepareCommandList();
				AnimationPass::Get().PrepareCommandList();
				SSAOPass::Get().PrepareCommandList();

				l_renderingServer->ExecuteCommandList(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics);

				l_renderingServer->WaitCommandQueue(OpaquePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(AnimationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);

				l_renderingServer->WaitCommandQueue(AnimationPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				//VolumetricPass::Render(false);

				LightPass::Get().PrepareCommandList();

				if (l_renderingConfig.drawSky)
				{
					SkyPass::Get().PrepareCommandList();
				}
				PreTAAPass::Get().PrepareCommandList();
				l_canvas = PreTAAPass::Get().GetResult();
				l_canvasOwner = PreTAAPass::Get().GetRenderPassComp();

				if (m_showTransparent)
				{
					TransparentGeometryProcessPass::Get().PrepareCommandList();

					TransparentBlendPassRenderingContext l_transparentBlendPassRenderingContext;
					l_transparentBlendPassRenderingContext.m_output = l_canvas;
					TransparentBlendPass::Get().PrepareCommandList(&l_transparentBlendPassRenderingContext);
				}

				l_renderingServer->WaitCommandQueue(SSAOPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				l_renderingServer->WaitCommandQueue(LightCullingPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				//l_renderingServer->WaitCommandQueue(SunShadowBlurEvenPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				l_renderingServer->ExecuteCommandList(LightPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(LightPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(LightPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				if (l_renderingConfig.drawSky)
				{
					l_renderingServer->ExecuteCommandList(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
					l_renderingServer->WaitCommandQueue(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
					l_renderingServer->ExecuteCommandList(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				}

				l_renderingServer->WaitCommandQueue(LightPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				l_renderingServer->WaitCommandQueue(SkyPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				l_renderingServer->ExecuteCommandList(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				l_renderingServer->WaitCommandQueue(PreTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				if (m_showTransparent)
				{
					l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
					l_renderingServer->WaitCommandQueue(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
					l_renderingServer->ExecuteCommandList(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
					l_renderingServer->WaitCommandQueue(TransparentGeometryProcessPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
					l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
					l_renderingServer->WaitCommandQueue(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
					l_renderingServer->ExecuteCommandList(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
				}
			}

			if (m_showVoxel)
			{
				l_canvas = VXGIRenderer::Get().GetVisualizationResult();
			}

			LuminanceHistogramPassRenderingContext l_LuminanceHistogramPassRenderingContext;
			l_LuminanceHistogramPassRenderingContext.m_input = l_canvas;
			LuminanceHistogramPass::Get().PrepareCommandList(&l_LuminanceHistogramPassRenderingContext);

			LuminanceAveragePass::Get().PrepareCommandList();

			if (m_showTransparent)
			{
				l_renderingServer->WaitCommandQueue(TransparentBlendPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			}
			l_renderingServer->ExecuteCommandList(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

			l_renderingServer->WaitCommandQueue(LuminanceHistogramPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_renderingServer->ExecuteCommandList(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Compute);

			if (l_renderingConfig.useTAA)
			{	
				TAAPassRenderingContext l_TAAPassRenderingContext;
				if (m_isPassOdd)
				{
					l_TAAPassRenderingContext.m_readTexture = TAAPass::Get().GetOddResult();
					l_TAAPassRenderingContext.m_writeTexture = TAAPass::Get().GetEvenResult();

					m_isPassOdd = false;
				}
				else
				{
					l_TAAPassRenderingContext.m_readTexture = TAAPass::Get().GetEvenResult();
					l_TAAPassRenderingContext.m_writeTexture = TAAPass::Get().GetOddResult();

					m_isPassOdd = true;
				}

				l_TAAPassRenderingContext.m_input = l_canvas;
				l_TAAPassRenderingContext.m_motionVector = OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3].m_Texture;

				TAAPass::Get().PrepareCommandList(&l_TAAPassRenderingContext);

				PostTAAPassRenderingContext l_PostTAAPassRenderingContext;
				l_PostTAAPassRenderingContext.m_input = l_TAAPassRenderingContext.m_writeTexture;
				PostTAAPass::Get().PrepareCommandList(&l_PostTAAPassRenderingContext);

				l_renderingServer->WaitCommandQueue(l_canvasOwner, GPUEngineType::Graphics, l_canvasOwner->m_RenderPassDesc.m_GPUEngineType);
				l_renderingServer->ExecuteCommandList(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				l_renderingServer->WaitCommandQueue(TAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
				l_renderingServer->ExecuteCommandList(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(PostTAAPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				l_canvas = PostTAAPass::Get().GetResult();
				l_canvasOwner = PostTAAPass::Get().GetRenderPassComp();
			}

			if (l_renderingConfig.useMotionBlur)
			{
				MotionBlurPassRenderingContext l_MotionBlurPassRenderingContext;
				l_MotionBlurPassRenderingContext.m_input = l_canvas;
				MotionBlurPass::Get().PrepareCommandList(&l_MotionBlurPassRenderingContext);

				l_renderingServer->WaitCommandQueue(l_canvasOwner, GPUEngineType::Graphics,  l_canvasOwner->m_RenderPassDesc.m_GPUEngineType);
				l_renderingServer->ExecuteCommandList(MotionBlurPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
				l_renderingServer->WaitCommandQueue(MotionBlurPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
				l_renderingServer->ExecuteCommandList(MotionBlurPass::Get().GetRenderPassComp(), GPUEngineType::Compute);

				l_canvas = MotionBlurPass::Get().GetResult();
				l_canvasOwner = MotionBlurPass::Get().GetRenderPassComp();
			}

			BillboardPass::Get().PrepareCommandList();
			DebugPass::Get().PrepareCommandList();

			l_renderingServer->ExecuteCommandList(BillboardPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(DebugPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);

			FinalBlendPassRenderingContext l_FinalBlendPassRenderingContext;
			l_FinalBlendPassRenderingContext.m_input = l_canvas;
			FinalBlendPass::Get().PrepareCommandList(&l_FinalBlendPassRenderingContext);

			l_renderingServer->WaitCommandQueue(l_canvasOwner, GPUEngineType::Graphics,  l_canvasOwner->m_RenderPassDesc.m_GPUEngineType);
			l_renderingServer->WaitCommandQueue(LuminanceAveragePass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);
			l_renderingServer->WaitCommandQueue(BillboardPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(DebugPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Graphics);

			l_renderingServer->ExecuteCommandList(FinalBlendPass::Get().GetRenderPassComp(), GPUEngineType::Graphics);
			l_renderingServer->WaitCommandQueue(FinalBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute, GPUEngineType::Graphics);
			l_renderingServer->ExecuteCommandList(FinalBlendPass::Get().GetRenderPassComp(), GPUEngineType::Compute);
			l_renderingServer->WaitCommandQueue(FinalBlendPass::Get().GetRenderPassComp(), GPUEngineType::Graphics, GPUEngineType::Compute);

			if (m_saveScreenCapture)
			{
				auto l_srcTextureComp = static_cast<TextureComponent*>(FinalBlendPass::Get().GetResult());
				auto l_textureData = l_renderingServer->ReadTextureBackToCPU(FinalBlendPass::Get().GetRenderPassComp(), l_srcTextureComp);
				g_Engine->getAssetSystem()->SaveTexture("ScreenCapture", l_srcTextureComp->m_TextureDesc, l_textureData.data());
				m_saveScreenCapture = false;
			}
		};

		f_TerminateJob = [&]() {
			DefaultGPUBuffers::Terminate();

			BRDFLUTPass::Get().Terminate();
			BRDFLUTMSPass::Get().Terminate();

			TiledFrustumGenerationPass::Get().Terminate();
			LightCullingPass::Get().Terminate();
			GIResolvePass::Terminate();
			SurfelGITestPass::Get().Terminate();
			LuminanceHistogramPass::Get().Terminate();
			LuminanceAveragePass::Get().Terminate();

			SunShadowGeometryProcessPass::Get().Terminate();
			SunShadowBlurOddPass::Get().Terminate();
			SunShadowBlurEvenPass::Get().Terminate();
			VXGIRenderer::Get().Terminate();
			OpaquePass::Get().Terminate();
			AnimationPass::Get().Terminate();

			SSAOPass::Get().Terminate();
			LightPass::Get().Terminate();
			SkyPass::Get().Terminate();
			PreTAAPass::Get().Terminate();
			TransparentGeometryProcessPass::Get().Terminate();
			TransparentBlendPass::Get().Terminate();
			VolumetricPass::Terminate();
			TAAPass::Get().Terminate();
			PostTAAPass::Get().Terminate();
			MotionBlurPass::Get().Terminate();
			BillboardPass::Get().Terminate();
			DebugPass::Get().Terminate();
			FinalBlendPass::Get().Terminate();

			BSDFTestPass::Get().Terminate();
		};

		auto l_DefaultRenderingClientSetupTask = g_Engine->getTaskSystem()->Submit("DefaultRenderingClientSetupTask", 2, nullptr, f_SetupJob);
		l_DefaultRenderingClientSetupTask.m_Future->Get();

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}

	bool DefaultRenderingClientImpl::Initialize()
	{
		auto l_DefaultRenderingClientInitializeTask = g_Engine->getTaskSystem()->Submit("DefaultRenderingClientInitializeTask", 2, nullptr, f_InitializeJob);
		l_DefaultRenderingClientInitializeTask.m_Future->Get();

		m_ObjectStatus = ObjectStatus::Activated;

		return true;
	}

	bool DefaultRenderingClientImpl::Render(IRenderingConfig* renderingConfig)
	{
		f_RenderJob();

		return true;
	}

	bool DefaultRenderingClientImpl::Terminate()
	{
		f_TerminateJob();

		m_ObjectStatus = ObjectStatus::Terminated;

		return true;
	}

	ObjectStatus DefaultRenderingClientImpl::GetStatus()
	{
		return m_ObjectStatus;
	}
}