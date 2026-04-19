#include "DevToggleRegistry.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace Inno
{
    namespace DevToggleRegistry
    {
        namespace
        {
            std::mutex                                g_mutex;
            std::unordered_map<std::string, Toggle>   g_toggles;
            std::unordered_map<std::string, Action>   g_actions;
        }

        void RegisterToggle(std::string name,
                            std::function<bool()> getter,
                            std::function<void(bool)> setter)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_toggles[name] = Toggle{ name, std::move(getter), std::move(setter) };
        }

        void RegisterAction(std::string name, std::function<void()> trigger)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_actions[name] = Action{ name, std::move(trigger) };
        }

        std::vector<Toggle> AllToggles()
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            std::vector<Toggle> out;
            out.reserve(g_toggles.size());
            for (auto& kv : g_toggles)
                out.push_back(kv.second);
            std::sort(out.begin(), out.end(),
                [](const Toggle& a, const Toggle& b) { return a.m_Name < b.m_Name; });
            return out;
        }

        std::vector<Action> AllActions()
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            std::vector<Action> out;
            out.reserve(g_actions.size());
            for (auto& kv : g_actions)
                out.push_back(kv.second);
            std::sort(out.begin(), out.end(),
                [](const Action& a, const Action& b) { return a.m_Name < b.m_Name; });
            return out;
        }

        bool Set(const std::string& name, bool value)
        {
            std::function<void(bool)> setter;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = g_toggles.find(name);
                if (it == g_toggles.end())
                    return false;
                setter = it->second.m_Set;
            }
            // Invoke outside the lock so the setter can do whatever it needs
            // (queue, write, etc.) without deadlocking against AllToggles
            // calls that the setter might trigger transitively.
            setter(value);
            return true;
        }

        bool Trigger(const std::string& name)
        {
            std::function<void()> trigger;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = g_actions.find(name);
                if (it == g_actions.end())
                    return false;
                trigger = it->second.m_Trigger;
            }
            trigger();
            return true;
        }

        void Clear()
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_toggles.clear();
            g_actions.clear();
        }
    }
}
