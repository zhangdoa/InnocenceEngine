#pragma once
#include "../../Engine/Interface/IRenderingClient.h"

namespace Inno
{
	// Single concrete client split across sibling _Section.cpp TUs (Setup,
	// Bootstrap, Hooks). The render graph owns every node + resource; this
	// client only registers the residual CPU hooks the data model can't
	// express, plus the bootstrap asset imports.
	class ExampleRenderingClient : public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ExampleRenderingClient);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

	private:
		void BootstrapAmbientCGTextures();
		void RegisterGraphHooks();

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}
