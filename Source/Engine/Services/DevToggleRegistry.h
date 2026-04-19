#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Inno
{
    // Lightweight registry of named developer toggles and one-shot actions
    // that any subsystem can publish and the editor (or other tooling) can
    // enumerate and control over IPC. Decouples EditorService from
    // example-project / client-specific classes — the client registers a
    // getter / setter pair, EditorService only knows about strings + bools.
    //
    // The setter is responsible for any frame-boundary deferral the client
    // needs (e.g. "queue toggle, apply at the start of next frame to avoid
    // mid-frame state churn"). The registry just plumbs the call through.
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

        // Wipe everything. Called on engine teardown so callbacks holding
        // references to soon-to-be-destroyed state don't outlive the owner.
        void Clear();
    }
}
