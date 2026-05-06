#include "EditorService_Internal.h"
#include "../Engine.h"
#include "../Common/LogService.h"
#include "SceneService.h"

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

namespace
{
	ix::WebSocketServer* GetServer(void* ptr) { return static_cast<ix::WebSocketServer*>(ptr); }

	json BuildErrorReply(uint64_t id, const std::string& code, const std::string& message)
	{
		json reply;
		reply["envelope"] = "reply";
		reply["id"]       = id;
		reply["status"]   = "err";
		reply["error"]    = { {"code", code}, {"message", message} };
		return reply;
	}

	json BuildOkReply(uint64_t id, json result)
	{
		json reply;
		reply["envelope"] = "reply";
		reply["id"]       = id;
		reply["status"]   = "ok";
		reply["result"]   = std::move(result);
		return reply;
	}

	json BuildEvent(const char* type, json payload)
	{
		json evt;
		evt["envelope"] = "event";
		evt["type"]     = type;
		evt["payload"]  = std::move(payload);
		return evt;
	}
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

// SCREENSHOT_SAVED payload schema (consumed by Source/Editor-Next/src/composables/useIpc.js):
//   { "ok": <bool>, "path": <string>, "error": <string> }
// Both "path" and "error" are always present so the editor doesn't need to
// branch on key existence; one of them is empty depending on "ok". On
// success: "path" is the absolute path AssetService::Save resolved to,
// "error" is "". On failure: "path" is "" if the save aborted before
// resolving a destination (or carries the intended path if the failure
// happened during write), "error" carries the failure reason.
bool EditorService::BroadcastScreenshotSaved(bool in_Ok, const std::string& in_AbsolutePath, const std::string& in_ErrorReason)
{
	if (!m_Server)
	{
		Log(Warning, "EditorService: BroadcastScreenshotSaved skipped — WS server is not running (ok=", in_Ok, ", path=\"", in_AbsolutePath.c_str(), "\", error=\"", in_ErrorReason.c_str(), "\").");
		return false;
	}
	auto l_server = GetServer(m_Server);
	json l_payload;
	l_payload["ok"]    = in_Ok;
	l_payload["path"]  = in_AbsolutePath;
	l_payload["error"] = in_ErrorReason;
	const auto l_msg = BuildEvent("SCREENSHOT_SAVED", std::move(l_payload)).dump();
	for (auto&& client : l_server->getClients())
		client->send(l_msg);
	Log(Verbose, "EditorService: Broadcast SCREENSHOT_SAVED event (ok=", in_Ok, ").");
	return true;
}

void EditorService::RegisterBuiltinHandlers()
{
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

	RegisterIntrospectionHandlers();
	RegisterDevAndSceneHandlers();
	RegisterEntityHandlers();
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
