#include "RayTracer_Internal.h"

#include "../Common/TaskScheduler.h"

#include "../Services/RenderingConfigurationService.h"
#include "../Services/TextureResourceService.h"

#include "../Engine.h"

using namespace Inno;

namespace RayTracerNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	std::atomic<bool> m_isWorking;
	SharedPtr<ITask> m_LastTask;
	const int m_maxDepth = 4;
	const int m_maxSamplePerPixel = 8;
	std::default_random_engine m_generator;
	std::uniform_real_distribution<float> m_randomDirDelta(-1.0f, 1.0f);

	TextureComponent* m_TextureComp;
	uint32_t m_outputWidth           = 0;
	uint32_t m_outputHeight          = 0;
	uint32_t m_explicitWidth         = 0;
	uint32_t m_explicitHeight        = 0;
	uint32_t m_downsampleDenominator = 8;

	// World-space toward-sun direction.
	Vec4 m_sunDir   = Vec4(0.0f, 1.0f, 0.0f, 0.0f);
	Vec4 m_sunColor = Vec4(1.0f, 0.95f, 0.8f, 1.0f);
}

bool RayTracer::Setup(IServiceConfig* systemConfig)
{
	auto* l_config = static_cast<RayTracerConfig*>(systemConfig);
	if (l_config)
	{
		RayTracerNS::m_explicitWidth         = l_config->outputWidth;
		RayTracerNS::m_explicitHeight        = l_config->outputHeight;
		RayTracerNS::m_downsampleDenominator = l_config->downsampleDenominator;
	}

	RayTracerNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RayTracer::Initialize()
{
	if (RayTracerNS::m_explicitWidth > 0 && RayTracerNS::m_explicitHeight > 0)
	{
		RayTracerNS::m_outputWidth  = RayTracerNS::m_explicitWidth;
		RayTracerNS::m_outputHeight = RayTracerNS::m_explicitHeight;
	}
	else
	{
		auto l_res = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		RayTracerNS::m_outputWidth  = l_res.x / RayTracerNS::m_downsampleDenominator;
		RayTracerNS::m_outputHeight = l_res.y / RayTracerNS::m_downsampleDenominator;
	}

	RayTracerNS::m_TextureComp = g_Engine->Get<TextureResourceService>()->Add("RayTracingResult");

	RayTracerNS::m_TextureComp->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	RayTracerNS::m_TextureComp->m_TextureDesc.Usage = TextureUsage::Sample;
	RayTracerNS::m_TextureComp->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;
	RayTracerNS::m_TextureComp->m_TextureDesc.Width = RayTracerNS::m_outputWidth;
	RayTracerNS::m_TextureComp->m_TextureDesc.Height = RayTracerNS::m_outputHeight;
	RayTracerNS::m_TextureComp->m_TextureDesc.PixelDataType = TexturePixelDataType::UByte;

	RayTracerNS::m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool RayTracer::Execute()
{
	if (!RayTracerNS::m_isWorking)
	{
		RayTracerNS::m_isWorking = true;

		RayTracerNS::m_LastTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("RayTracingTask", ITask::Type::Once, 4), [&]() { ExecuteRayTracing(); RayTracerNS::m_isWorking = false; });
		RayTracerNS::m_LastTask->Activate();
	}

	return true;
}

bool RayTracer::Terminate()
{
	if (RayTracerNS::m_LastTask)
	{
		// Must complete before TaskScheduler::Freeze/Reset — otherwise this Wait() deadlocks.
		RayTracerNS::m_LastTask->Wait();
	}
	RayTracerNS::m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus RayTracer::GetStatus()
{
	return RayTracerNS::m_ObjectStatus;
}
