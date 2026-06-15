#include "PerFrameDataService_Toggles.h"

#include "ConfigurationService.h"
#include "DevToggleRegistry.h"

#include "../Engine.h"

using namespace Inno;

void Inno::RegisterDevTogglesForPerFrameData(PerFrameDataService& svc)
{
    for (const auto& l_entry : g_Engine->Get<ConfigurationService>()->GetDebugViewModes())
    {
        const auto l_mode = static_cast<DebugViewMode>(l_entry.modeId);
        DevToggleRegistry::RegisterToggle(l_entry.name.c_str(),
            [&svc, l_mode]() { return svc.GetDebugViewMode() == l_mode; },
            [&svc, l_mode](bool desired)
            {
                if (desired)
                    svc.SetDebugViewMode(l_mode);
                else if (svc.GetDebugViewMode() == l_mode)
                    svc.SetDebugViewMode(DebugViewMode::None);
            });
    }

    DevToggleRegistry::RegisterToggle("PointShadowBypass",
        [&svc]() { return svc.GetPointShadowBypass(); },
        [&svc](bool desired) { svc.SetPointShadowBypass(desired); });
}
