#include "ExampleRenderingClient_Internal.h"
#include "../../Engine/Common/Array.h"

#include "../../Engine/Services/DevToggleRegistry.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	bool ExampleRenderingClientImpl::Initialize()
	{
		// The render graph owns every node, resource, and per-frame update (via
		// node data + registered init/update hooks). No render-pass C++ classes.
		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	bool ExampleRenderingClientImpl::Update()
	{
		return true;
	}

	bool ExampleRenderingClientImpl::FinalizeGPUResults()
	{
		// Structural fallback for the per-frame auto-capture trigger in
		// ExecuteCommands. Engine::Terminate calls this AFTER WaitForGPUIdle
		// and BEFORE the LogicClient CPU path tracer, so the GPU is guaranteed
		// alive and a readback/PNG write here is safe. Per-frame trigger
		// usually wins (m_autoCaptureWritten is already set); this catches
		// the "user exited before the trigger frame fired" case.
		// Serialize-test mode sets totalFrames=1 as an auto-terminate signal
		// but doesn't render; skip the capture path so it doesn't try to
		// read back an unactivated texture at shutdown.
		const bool l_isSerializeTest = g_Engine->getInitConfig().serializeTest[0] != '\0';
		if (g_Engine->getInitConfig().totalFrames > 0 && !m_autoCaptureWritten && !l_isSerializeTest)
			TryWriteAutoCapture();
		return true;
	}

	bool ExampleRenderingClientImpl::Terminate()
	{
		// Registered callbacks capture `this`; clear the registry before this
		// instance starts to die so an in-flight WS message can't deref it.
		DevToggleRegistry::Clear();

		auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
		auto l_graphicsSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = l_hwService->GetSemaphoreValue(GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		l_hwService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);



		m_ObjectStatus = ObjectStatus::Terminated;

		return true;
	}

	ObjectStatus ExampleRenderingClientImpl::GetStatus()
	{
		return m_ObjectStatus;
	}
}

bool ExampleRenderingClient::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new ExampleRenderingClientImpl();
	return m_Impl->Setup(systemConfig);
}

bool ExampleRenderingClient::Initialize()
{
	return m_Impl->Initialize();
}

bool ExampleRenderingClient::Update()
{
	return m_Impl->Update();
}

bool ExampleRenderingClient::PrepareCommands()
{
	return m_Impl->PrepareCommands();
}

bool ExampleRenderingClient::ExecuteCommands(IRenderingConfig* renderingConfig)
{
	return m_Impl->ExecuteCommands(renderingConfig);
}

bool ExampleRenderingClient::FinalizeGPUResults()
{
	return m_Impl->FinalizeGPUResults();
}

bool ExampleRenderingClient::Terminate()
{
	if (m_Impl->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

Inno::Array<IRenderPass*> ExampleRenderingClient::GetDispatchedPasses() const
{
	// All render passes are now graph JSON nodes with no C++ class, so there are
	// no IRenderPass objects to expose to the editor inspector.
	return {};
}

ObjectStatus ExampleRenderingClient::GetStatus()
{
	return m_Impl->GetStatus();
}
