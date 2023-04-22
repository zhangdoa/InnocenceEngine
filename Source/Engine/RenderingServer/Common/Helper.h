#pragma once
#include "../../Component/TextureComponent.h"
#include "../../Component/RenderPassComponent.h"
#include "../../Component/ShaderProgramComponent.h"
#include "../IRenderingServer.h"

namespace Inno
{
    namespace RenderingServerHelper
    {
        bool ReserveRenderTargets(RenderPassComponent* RenderPassComp, IRenderingServer* renderingServer);
        bool CreateRenderTargets(RenderPassComponent* RenderPassComp, IRenderingServer* renderingServer);
    }
}