#include "../Engine/Interface/IClientFactory.h"
#include "TestLogicClient.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient()
    {
        return nullptr;
    }

    std::unique_ptr<ILogicClient> CreateLogicClient()
    {
        return std::make_unique<TestLogicClient>();
    }
}
