#include "../Engine/Interface/IClientFactory.h"
#include "TestRenderingClient.h"
#include "TestLogicClient.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient()
    {
        return std::make_unique<TestRenderingClient>();
    }

    std::unique_ptr<ILogicClient> CreateLogicClient()
    {
        return std::make_unique<TestLogicClient>();
    }
}
