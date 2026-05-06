#include "EditorService_Internal.h"
#include "../Engine.h"
#include "../Common/LogService.h"
#include "DevToggleRegistry.h"
#include "ViewportSourceOverride.h"
#include "SceneService.h"
#include "AssetService.h"

using namespace Inno;

void EditorService::RegisterDevAndSceneHandlers()
{
	auto reg = [this](const char* type, EditorServiceImpl::Handler h) {
		std::lock_guard<std::mutex> lock(m_Impl->mutex);
		m_Impl->handlers[type] = std::move(h);
	};

	reg("SET_DEV_TOGGLE", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "name", "value" });
		const std::string l_name  = payload["name"];
		const bool        l_value = payload["value"];
		if (!DevToggleRegistry::Set(l_name, l_value))
			throw EditorReqError("NOT_FOUND", "Unknown dev toggle: " + l_name);
		// Read back the actual state — the setter may coerce, defer,
		// or otherwise land on a value different from what was asked.
		// Reply with truth so the client doesn't stay out of sync.
		auto l_actual = DevToggleRegistry::Get(l_name);
		return json{
			{"name",  l_name},
			{"value", l_actual.value_or(l_value)},
		};
	});

	reg("TRIGGER_DEV_ACTION", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "name" });
		const std::string l_name = payload["name"];
		if (!DevToggleRegistry::Trigger(l_name))
			throw EditorReqError("NOT_FOUND", "Unknown dev action: " + l_name);
		return json{ {"name", l_name} };
	});

	reg("SET_VIEWPORT_SOURCE", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		if (payload.contains("pass") && payload.contains("rtIndex"))
		{
			const std::string l_pass    = payload["pass"];
			const uint32_t    l_rtIndex = payload["rtIndex"].get<uint32_t>();
			ViewportSourceOverride::Set(l_pass, l_rtIndex);
		}
		else
		{
			ViewportSourceOverride::Reset();
		}
		// Read back the actual override state so the client commits truth,
		// not the payload it submitted. Empty object means no override.
		auto l_current = ViewportSourceOverride::Get();
		if (l_current.has_value())
			return json{ {"pass", l_current->m_PassName}, {"rtIndex", l_current->m_RTIndex} };
		return json::object();
	});

	reg("LOAD_SCENE", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "path" });
		const std::string l_path = payload["path"];
		Log(Success, "EditorService: Requesting scene load: ", l_path.c_str());
		// AsyncLoad=true: this callback runs on the WebSocket worker thread;
		// SceneService::Load from a non-main thread must be async or it races
		// DX12 resource lifecycle on the render thread.
		g_Engine->Get<SceneService>()->Load(l_path.c_str(), true);
		return json{ {"path", l_path} };
	});

	reg("SAVE_SCENE", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
		Log(Success, "EditorService: Requesting scene save.");
		auto sceneService = g_Engine->Get<SceneService>();
		const bool ok = sceneService->Save(sceneService->GetCurrentSceneName().c_str());
		if (!ok)
			throw EditorReqError("SAVE_FAILED", "Scene save failed");
		return json{ {"path", sceneService->GetCurrentSceneName().c_str()} };
	});

	reg("IMPORT_ASSET", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "path" });
		const std::string l_path = payload["path"];
		Log(Success, "EditorService: Requesting asset import: ", l_path.c_str());
		g_Engine->Get<AssetService>()->Import(l_path.c_str());
		return json{ {"path", l_path} };
	});
}
