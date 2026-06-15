#include "ExampleRenderingClient.h"

#include "../../Engine/RenderGraph/RenderGraphService.h"
#include "../../Engine/Services/ConfigurationService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

namespace Inno
{
	bool ExampleRenderingClient::Setup(IServiceConfig* systemConfig)
	{
		BootstrapAmbientCGTextures();

		// The graph owns every node + resource. RegisterGraphHooks installs the
		// named init/update hooks (imported-resource creation, per-frame uploads)
		// the data model can't express; LoadGraph then runs the init hooks.
		RegisterGraphHooks();
		const auto& l_graphFile = g_Engine->Get<ConfigurationService>()->GetRenderGraphFile();
		g_Engine->Get<RenderGraphService>()->LoadGraph(
			l_graphFile.empty() ? "ExampleProject/RenderGraph/ExampleRenderGraph.json" : l_graphFile.c_str());

		m_ObjectStatus = ObjectStatus::Created;

		return true;
	}
}
