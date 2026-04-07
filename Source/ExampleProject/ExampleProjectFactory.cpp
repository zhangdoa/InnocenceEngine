#include "../Engine/Interface/IClientFactory.h"
#include "RenderingClient/ExampleRenderingClient.h"
#include "LogicClient/ExampleLogicClient.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient()
    {
        return std::make_unique<ExampleRenderingClient>();
    }

    std::unique_ptr<ILogicClient> CreateLogicClient()
    {
        return std::make_unique<ExampleLogicClient>();
    }
}
