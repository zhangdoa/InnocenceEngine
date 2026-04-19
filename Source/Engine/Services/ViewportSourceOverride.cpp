#include "ViewportSourceOverride.h"

#include <mutex>

namespace Inno
{
    namespace ViewportSourceOverride
    {
        namespace
        {
            std::mutex                   g_mutex;
            std::optional<Selection>     g_selection;
        }

        void Set(std::string passName, uint32_t rtIndex)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_selection = Selection{ std::move(passName), rtIndex };
        }

        void Reset()
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_selection.reset();
        }

        std::optional<Selection> Get()
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            return g_selection;
        }
    }
}
