#pragma once
#include "PerFrameDataService.h"

namespace Inno
{
    // Register the dev toggles that drive PerFrameDataService's atomic
    // state (DebugViewMode + PointShadowBypass). Names and mode ids come
    // from the loaded config; see ConfigurationService.GetDebugViewModes.
    void RegisterDevTogglesForPerFrameData(PerFrameDataService& svc);
}
