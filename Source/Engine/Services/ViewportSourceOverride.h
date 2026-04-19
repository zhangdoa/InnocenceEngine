#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace Inno
{
    // Tooling-driven override for "what texture feeds the viewport". When
    // set, the rendering client substitutes the named pass's color RT for
    // the default presentation source. Reset() clears the override.
    namespace ViewportSourceOverride
    {
        struct Selection
        {
            std::string m_PassName;
            uint32_t    m_RTIndex;
        };

        void                       Set(std::string passName, uint32_t rtIndex);
        void                       Reset();
        std::optional<Selection>   Get();
    }
}
