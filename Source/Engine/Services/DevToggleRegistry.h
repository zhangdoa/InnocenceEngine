#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace Inno
{
    // Named developer toggles and one-shot actions, addressable by string
    // from any thread. Subsystems publish via Register*; tooling enumerates
    // via All* and drives via Set / Trigger.
    //
    // Setters are responsible for any frame-boundary deferral they need —
    // the registry calls them directly, on the caller's thread.
    namespace DevToggleRegistry
    {
        struct Toggle
        {
            std::string                m_Name;
            std::function<bool()>      m_Get;
            std::function<void(bool)>  m_Set;
        };

        struct Action
        {
            std::string             m_Name;
            std::function<void()>   m_Trigger;
        };

        void RegisterToggle(std::string name, std::function<bool()> getter, std::function<void(bool)> setter);
        void RegisterAction(std::string name, std::function<void()> trigger);

        std::vector<Toggle> AllToggles();
        std::vector<Action> AllActions();

        // Returns false if the name is not registered. Both Set and Trigger
        // are safe to call from any thread; the registry takes a lock and
        // the per-toggle setter is expected to be thread-safe (typically
        // implemented as "set an atomic / push to a queue").
        bool Set(const std::string& name, bool value);
        bool Trigger(const std::string& name);

        // Read current state by name, via the toggle's registered getter.
        // Returns nullopt when the name is not registered or the getter
        // is missing. Useful after Set() so callers can observe whatever
        // value the setter actually landed on (coerced, clamped, queued).
        std::optional<bool> Get(const std::string& name);

        // Wipe everything. Called on engine teardown so callbacks holding
        // references to soon-to-be-destroyed state don't outlive the owner.
        void Clear();
    }
}
