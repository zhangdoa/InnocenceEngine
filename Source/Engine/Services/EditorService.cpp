#include "EditorService.h"
#include "../Engine.h"
#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "AssetService.h"
#include "DevToggleRegistry.h"
#include "RenderPassResourceService.h"
#include "ViewportSourceOverride.h"
#include "../Common/TaskScheduler.h"
#include "../Common/Thread.h"
#include "../Component/TransformComponent.h"
#include "../Component/LightComponent.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

#ifdef INNO_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXWebSocket.h>

#include <functional>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

using namespace Inno;

// Editor ↔ engine wire protocol (mirrored in Source/Editor-Next/src/composables/useIpc.js).
//
//   REQUEST  { envelope: "request", id: N, type: "...", payload: {...} }
//   REPLY    { envelope: "reply",   id: N, status: "ok"|"err", result? | error? }
//   EVENT    { envelope: "event",   type: "...", payload: {...} }
//
// Handlers take the payload and return a result json; the dispatcher wraps it
// in the reply envelope with the matching id. Throwing `EditorReqError`
// translates into a status=err reply without crashing the WS loop.

namespace Inno
{
	class EditorReqError : public std::runtime_error
	{
	public:
		EditorReqError(std::string code, std::string message)
			: std::runtime_error(message), m_Code(std::move(code))
		{
		}
		const std::string& Code() const { return m_Code; }
	private:
		std::string m_Code;
	};

	struct EditorServiceImpl
	{
		using Handler = std::function<json(const json& payload, ix::WebSocket& ws)>;
		std::mutex                                  mutex;
		std::unordered_map<std::string, Handler>    handlers;
	};
}

inline ix::WebSocketServer* GetServer(void* ptr) { return static_cast<ix::WebSocketServer*>(ptr); }

static json SerializeVec(const Vec3& v) { return { v.x, v.y, v.z }; }
static json SerializeVec(const Vec4& v) { return { v.x, v.y, v.z, v.w }; }

static json BuildErrorReply(uint64_t id, const std::string& code, const std::string& message)
{
	json reply;
	reply["envelope"] = "reply";
	reply["id"]       = id;
	reply["status"]   = "err";
	reply["error"]    = { {"code", code}, {"message", message} };
	return reply;
}

static json BuildOkReply(uint64_t id, json result)
{
	json reply;
	reply["envelope"] = "reply";
	reply["id"]       = id;
	reply["status"]   = "ok";
	reply["result"]   = std::move(result);
	return reply;
}

static json BuildEvent(const char* type, json payload)
{
	json evt;
	evt["envelope"] = "event";
	evt["type"]     = type;
	evt["payload"]  = std::move(payload);
	return evt;
}

EditorService::EditorService() = default;
EditorService::~EditorService() = default;

bool EditorService::Setup(IServiceConfig* config)
{
	m_Impl = std::make_unique<EditorServiceImpl>();
	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "EditorService: Setup finished.");
	return true;
}

bool EditorService::Initialize()
{
	auto l_server = new ix::WebSocketServer(8081, "127.0.0.1");
	m_Server = l_server;

	RegisterBuiltinHandlers();

	l_server->setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> /*connectionState*/, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
		{
			if (msg->type == ix::WebSocketMessageType::Open)
			{
				Log(Success, "EditorService: New client connection opened.");
				return;
			}
			if (msg->type == ix::WebSocketMessageType::Close)
			{
				Log(Success, "EditorService: Client connection closed.");
				return;
			}
			if (msg->type == ix::WebSocketMessageType::Error)
			{
				Log(Error, "EditorService: WebSocket error: ", msg->errorInfo.reason.c_str());
				return;
			}
			if (msg->type != ix::WebSocketMessageType::Message)
				return;

			json l_json;
			try { l_json = json::parse(msg->str); }
			catch (const std::exception& e)
			{
				Log(Error, "EditorService: Failed to parse JSON: ", e.what());
				return;
			}

			// Enforce the envelope contract. Clients that send bare {type:...}
			// messages pre-TASK-86 are silently dropped rather than routed —
			// the editor rewrite deletes the Gemini-era compat path.
			const std::string l_envelope = l_json.value("envelope", std::string());
			if (l_envelope != "request")
			{
				Log(Warning, "EditorService: ignoring non-request envelope: \"", l_envelope.c_str(), "\"");
				return;
			}

			if (!l_json.contains("id") || !l_json.contains("type"))
			{
				Log(Warning, "EditorService: request missing required id/type; dropping.");
				return;
			}

			const uint64_t    l_id   = l_json["id"].get<uint64_t>();
			const std::string l_type = l_json["type"].get<std::string>();
			const json        l_payload = l_json.value("payload", json::object());

			EditorServiceImpl::Handler l_handler;
			{
				std::lock_guard<std::mutex> lock(m_Impl->mutex);
				auto it = m_Impl->handlers.find(l_type);
				if (it != m_Impl->handlers.end())
					l_handler = it->second;
			}
			if (!l_handler)
			{
				Log(Warning, "EditorService: no handler for request type \"", l_type.c_str(), "\" (id=", l_id, ")");
				webSocket.send(BuildErrorReply(l_id, "NO_HANDLER", "No handler registered for request type: " + l_type).dump());
				return;
			}

			try
			{
				json l_result = l_handler(l_payload, webSocket);
				webSocket.send(BuildOkReply(l_id, std::move(l_result)).dump());
			}
			catch (const EditorReqError& e)
			{
				Log(Warning, "EditorService: handler \"", l_type.c_str(), "\" rejected id=", l_id, ": ", e.what());
				webSocket.send(BuildErrorReply(l_id, e.Code(), e.what()).dump());
			}
			catch (const std::exception& e)
			{
				Log(Error, "EditorService: handler \"", l_type.c_str(), "\" threw on id=", l_id, ": ", e.what());
				webSocket.send(BuildErrorReply(l_id, "INTERNAL", e.what()).dump());
			}
		});

	auto l_res = l_server->listen();
	if (!l_res.first)
	{
		Log(Error, "EditorService: Failed to start WebSocket server: ", l_res.second.c_str());
		delete l_server;
		m_Server = nullptr;
		return false;
	}

	l_server->start();

	// Every scene load (editor-initiated or engine-initiated) ends by running
	// the registered callbacks; ours broadcasts SCENE_UPDATED so connected
	// editors refresh their hierarchy without the client having to poll.
	m_sceneLoadedCallback = [this]() { BroadcastSceneUpdated(); };
	g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&m_sceneLoadedCallback);

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "EditorService: WebSocket server started on port 8081.");
	return true;
}

void EditorService::BroadcastSceneUpdated()
{
	if (!m_Server) return;
	auto l_server = GetServer(m_Server);
	json payload;
	payload["scene"] = g_Engine->Get<SceneService>()->GetCurrentSceneName();
	const auto l_msg = BuildEvent("SCENE_UPDATED", std::move(payload)).dump();
	for (auto&& client : l_server->getClients())
		client->send(l_msg);
	Log(Verbose, "EditorService: Broadcast SCENE_UPDATED event.");
}

static void RequireFields(const json& payload, std::initializer_list<const char*> fields)
{
	for (auto* f : fields)
	{
		if (!payload.contains(f))
			throw EditorReqError("BAD_PAYLOAD", std::string("missing required field: ") + f);
	}
}

void EditorService::RegisterBuiltinHandlers()
{
	auto reg = [this](const char* type, EditorServiceImpl::Handler h) {
		std::lock_guard<std::mutex> lock(m_Impl->mutex);
		m_Impl->handlers[type] = std::move(h);
	};

	// Setter-reply contract (SET_* and any other mutating handler whose reply
	// the client commits as authoritative state): the reply MUST carry the
	// post-mutation state read back from the authoritative source — never the
	// payload the client submitted. Read-back is the only way to confirm the
	// engine actually applied the mutation; echoing the payload masks
	// rejection, clamping, and async-deferral bugs (the client commits its
	// own guess as truth, then drifts from the engine on every silent
	// divergence). Prior-art fixes: SET_DEV_TOGGLE (commit 2586477b) and
	// SET_VIEWPORT_SOURCE (commit d4fe5462) both regressed under the
	// payload-echo pattern before being switched to read-back.

	reg("HELLO", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
		// Aliveness handshake. The editor (Electron main.js) sends this
		// after the WS opens; the reply just confirms the dispatcher is
		// running. No payload to share — the editor does not embed the
		// engine's frames, so no shared-texture handle is exchanged.
		return json{ {"ok", true} };
	});

	reg("GET_SCENE", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
		auto l_registry = g_Engine->Get<EntityRegistry>();
		auto l_ids = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
		json l_entities = json::array();
		for (auto l_id : l_ids)
		{
			json l_entity;
			l_entity["id"]   = (uint32_t)l_id;
			l_entity["name"] = l_registry->GetName(l_id);
			l_entities.push_back(l_entity);
		}
		json result;
		result["entities"] = l_entities;
		return result;
	});

	reg("GET_ENTITY_DETAILS", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "id" });
		EntityID l_id = (EntityID)payload["id"].get<uint32_t>();
		auto l_registry = g_Engine->Get<EntityRegistry>();
		if (!l_registry->IsValid(l_id))
			throw EditorReqError("NOT_FOUND", "Entity does not exist or has been destroyed");

		json l_details;
		l_details["id"]   = (uint32_t)l_id;
		l_details["name"] = l_registry->GetName(l_id);

		json l_components = json::array();

		auto l_transform = l_registry->Get<TransformComponent>(l_id);
		if (l_transform)
		{
			json l_comp;
			l_comp["type"]  = "TransformComponent";
			l_comp["pos"]   = SerializeVec(l_transform->m_LocalPos);
			l_comp["rot"]   = SerializeVec(l_transform->m_LocalRot);
			l_comp["scale"] = SerializeVec(l_transform->m_LocalScale);
			l_components.push_back(l_comp);
		}

		auto l_light = l_registry->Get<LightComponent>(l_id);
		if (l_light)
		{
			json l_comp;
			l_comp["type"]       = "LightComponent";
			l_comp["color"]      = SerializeVec(l_light->m_RGBColor);
			l_comp["shape"]      = SerializeVec(l_light->m_Shape);
			l_comp["lightType"]  = (int)l_light->m_LightType;
			l_comp["intensity"]  = l_light->m_LuminousFlux;
			l_comp["castShadow"] = l_light->m_CastShadow;
			l_components.push_back(l_comp);
		}

		l_details["components"] = l_components;

		json result;
		result["details"] = l_details;
		return result;
	});

	reg("LIST_DEV_TOGGLES", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
		json l_toggles = json::array();
		for (auto& t : DevToggleRegistry::AllToggles())
		{
			json e;
			e["name"]  = t.m_Name;
			e["value"] = t.m_Get ? t.m_Get() : false;
			l_toggles.push_back(e);
		}
		json l_actions = json::array();
		for (auto& a : DevToggleRegistry::AllActions())
			l_actions.push_back({{"name", a.m_Name}});

		json result;
		result["toggles"] = l_toggles;
		result["actions"] = l_actions;
		return result;
	});

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

	reg("LIST_TASKS", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
		auto* l_scheduler = g_Engine->Get<TaskScheduler>();
		json l_threads = json::array();
		size_t l_threadCount = l_scheduler->GetThreadCounts();
		for (uint32_t t = 0; t < l_threadCount; ++t)
		{
			const auto& l_buffer = l_scheduler->GetTaskReport(t);
			json l_reports = json::array();
			for (size_t i = 0; i < l_buffer.size(); ++i)
			{
				const auto& l_r = l_buffer[i];
				if (!l_r.m_TaskName)
					continue;
				json l_report;
				l_report["name"]       = l_r.m_TaskName;
				l_report["startTime"]  = l_r.m_StartTime;
				l_report["finishTime"] = l_r.m_FinishTime;
				l_reports.push_back(l_report);
			}
			json l_thread;
			l_thread["index"]   = t;
			l_thread["reports"] = l_reports;
			l_threads.push_back(l_thread);
		}
		json result;
		result["threads"] = l_threads;
		return result;
	});

	reg("LIST_RENDER_TARGETS", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
		json l_passes = json::array();
		g_Engine->Get<RenderPassResourceService>()->ForEach(
			[&l_passes](RenderPassComponent* rp)
			{
				if (!rp || !rp->m_OutputMergerTarget)
					return;
				json l_rts = json::array();
				for (size_t i = 0; i < rp->m_OutputMergerTarget->m_ColorOutputs.size(); ++i)
				{
					auto* tex = rp->m_OutputMergerTarget->m_ColorOutputs[i];
					if (!tex)
						continue;
					json l_rt;
					l_rt["index"] = static_cast<uint32_t>(i);
					l_rt["name"]  = tex->m_InstanceName.c_str();
					l_rts.push_back(l_rt);
				}
				if (l_rts.empty())
					return;
				json l_pass;
				l_pass["name"]    = rp->m_InstanceName.c_str();
				l_pass["targets"] = l_rts;
				l_passes.push_back(l_pass);
			});

		json result;
		result["passes"] = l_passes;
		auto l_current = ViewportSourceOverride::Get();
		if (l_current.has_value())
		{
			json l_sel;
			l_sel["pass"]    = l_current->m_PassName;
			l_sel["rtIndex"] = l_current->m_RTIndex;
			result["override"] = l_sel;
		}
		else
		{
			result["override"] = nullptr;
		}
		return result;
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

	reg("ENTITY_CREATE", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		const std::string l_name = payload.value("name", std::string("Entity"));
		auto* l_registry = g_Engine->Get<EntityRegistry>();
		auto  l_id = l_registry->Spawn(ObjectLifespan::Scene, l_name.c_str());

		auto l_ids = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
		json l_entities = json::array();
		for (auto id : l_ids)
		{
			json e;
			e["id"]   = (uint32_t)id;
			e["name"] = l_registry->GetName(id);
			l_entities.push_back(e);
		}
		json result;
		result["entity"]   = { {"id", (uint32_t)l_id}, {"name", l_name} };
		result["entities"] = l_entities;
		return result;
	});

	reg("ENTITY_DELETE", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "id" });
		auto* l_registry = g_Engine->Get<EntityRegistry>();
		const EntityID l_id = (EntityID)payload["id"].get<uint32_t>();
		if (!l_registry->IsValid(l_id))
			throw EditorReqError("NOT_FOUND", "Entity does not exist or has been destroyed");
		// Refuse to delete persistent (engine-owned) entities; the editor only
		// owns scene-bound entities. Otherwise a stray click could nuke the
		// player camera, sun, etc.
		if (l_registry->GetLifespan(l_id) != ObjectLifespan::Scene)
			throw EditorReqError("FORBIDDEN", "Refusing to delete non-scene entity");
		l_registry->Destroy(l_id);

		auto l_ids = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
		json l_entities = json::array();
		for (auto id : l_ids)
		{
			json e;
			e["id"]   = (uint32_t)id;
			e["name"] = l_registry->GetName(id);
			l_entities.push_back(e);
		}
		json result;
		result["removed"]  = (uint32_t)l_id;
		result["entities"] = l_entities;
		return result;
	});

	reg("ENTITY_RENAME", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "id", "name" });
		auto* l_registry = g_Engine->Get<EntityRegistry>();
		const EntityID    l_id   = (EntityID)payload["id"].get<uint32_t>();
		const std::string l_name = payload["name"];
		if (!l_registry->Rename(l_id, l_name.c_str()))
			throw EditorReqError("RENAME_FAILED", "Rename failed; entity may not exist");

		auto l_ids = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
		json l_entities = json::array();
		for (auto id : l_ids)
		{
			json e;
			e["id"]   = (uint32_t)id;
			e["name"] = l_registry->GetName(id);
			l_entities.push_back(e);
		}
		json result;
		result["renamed"]  = (uint32_t)l_id;
		result["name"]     = l_name;
		result["entities"] = l_entities;
		return result;
	});

	reg("UPDATE_ENTITY_PROPERTY", [](const json& payload, ix::WebSocket& /*ws*/) -> json {
		RequireFields(payload, { "id", "component", "property", "value" });
		const EntityID    l_id       = (EntityID)payload["id"].get<uint32_t>();
		const std::string l_compType = payload["component"];
		const std::string l_prop     = payload["property"];
		auto              l_val      = payload["value"];

		auto* l_registry = g_Engine->Get<EntityRegistry>();
		if (!l_registry->IsValid(l_id))
			throw EditorReqError("NOT_FOUND", "Entity does not exist or has been destroyed");

		json committed;
		if (l_compType == "TransformComponent")
		{
			auto l_transform = l_registry->Get<TransformComponent>(l_id);
			if (!l_transform)
				throw EditorReqError("NO_COMPONENT", "Entity has no TransformComponent");
			if (l_prop == "pos")
			{
				l_transform->m_LocalPos = Vec3(l_val[0], l_val[1], l_val[2]);
				committed = SerializeVec(l_transform->m_LocalPos);
			}
			else if (l_prop == "scale")
			{
				l_transform->m_LocalScale = Vec3(l_val[0], l_val[1], l_val[2]);
				committed = SerializeVec(l_transform->m_LocalScale);
			}
			else if (l_prop == "rot")
			{
				l_transform->m_LocalRot = Vec4(l_val[0], l_val[1], l_val[2], l_val[3]);
				committed = SerializeVec(l_transform->m_LocalRot);
			}
			else
			{
				throw EditorReqError("BAD_PROPERTY", "Unknown TransformComponent property: " + l_prop);
			}
		}
		else if (l_compType == "LightComponent")
		{
			auto l_light = l_registry->Get<LightComponent>(l_id);
			if (!l_light)
				throw EditorReqError("NO_COMPONENT", "Entity has no LightComponent");
			if (l_prop == "intensity")
			{
				l_light->m_LuminousFlux = l_val.get<float>();
				committed = l_light->m_LuminousFlux;
			}
			else if (l_prop == "color")
			{
				// An explicit color edit is a user signal to leave K-mode;
				// otherwise LightSimulationService::Update() overwrites the
				// commit with ColorTemperatureToRGB(m_ColorTemperature) on
				// the next frame and the inspector readback drifts back to
				// the K-derived value (TASK-184).
				l_light->m_RGBColor = Vec4(l_val[0], l_val[1], l_val[2], 1.0f);
				l_light->m_UseColorTemperature = false;
				committed = SerializeVec(l_light->m_RGBColor);
			}
			else if (l_prop == "castShadow")
			{
				l_light->m_CastShadow = l_val.get<bool>();
				committed = l_light->m_CastShadow;
			}
			else if (l_prop == "shape" || l_prop == "lightType")
			{
				// GET_ENTITY_DETAILS exposes these for inspector display, but
				// the inspector has no editor for them — so the write path is
				// intentionally absent rather than missing. READ_ONLY is the
				// discriminated reply that lets clients (and the symmetry
				// regression spec) distinguish "not yet wired" from "by design
				// not editable". Adding a writer means deleting this branch.
				throw EditorReqError("READ_ONLY", "LightComponent." + l_prop + " is read-only");
			}
			else
			{
				throw EditorReqError("BAD_PROPERTY", "Unknown LightComponent property: " + l_prop);
			}
		}
		else
		{
			throw EditorReqError("BAD_COMPONENT", "Unknown component type: " + l_compType);
		}

		return json{
			{"id", (uint32_t)l_id},
			{"component", l_compType},
			{"property", l_prop},
			{"value", committed},
		};
	});
}

bool EditorService::Update()
{
	return true;
}

bool EditorService::Terminate()
{
	if (m_Server)
	{
		auto l_server = GetServer(m_Server);
		l_server->stop();
		delete l_server;
		m_Server = nullptr;
	}
	if (m_Impl)
	{
		std::lock_guard<std::mutex> lock(m_Impl->mutex);
		m_Impl->handlers.clear();
	}
	m_Impl.reset();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "EditorService: Terminated.");
	return true;
}

ObjectStatus EditorService::GetStatus()
{
	return m_ObjectStatus;
}

