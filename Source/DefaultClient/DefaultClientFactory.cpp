#include "../Engine/Interface/IClientFactory.h"
#include "RenderingClient/DefaultRenderingClient.h"
#include "LogicClient/DefaultLogicClient.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient()
    {
        return std::make_unique<DefaultRenderingClient>();
    }

    std::unique_ptr<ILogicClient> CreateLogicClient()
    {
        return std::make_unique<DefaultLogicClient>();
    }
}
