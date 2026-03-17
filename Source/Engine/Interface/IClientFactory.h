#pragma once
#include "IRenderingClient.h"
#include "ILogicClient.h"
#include "../Common/STL14.h"

namespace Inno
{
    std::unique_ptr<IRenderingClient> CreateRenderingClient();
    std::unique_ptr<ILogicClient>     CreateLogicClient();
}
