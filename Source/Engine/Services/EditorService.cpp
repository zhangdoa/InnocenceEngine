#include "EditorService.h"
#include "../Engine.h"
#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "AssetService.h"
#include "RenderingConfigurationService.h"
#include "FrameManagementService.h"
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
#include <unordered_map>

using namespace Inno;

namespace Inno
{
	struct EditorServiceImpl
	{
		using Handler = std::function<void(const json& msg, ix::WebSocket& ws)>;
		std::mutex                                  mutex;
		std::unordered_map<std::string, Handler>    handlers;
	};
}

inline ix::WebSocketServer* GetServer(void* ptr) { return static_cast<ix::WebSocketServer*>(ptr); }

static json SerializeVec(const Vec3& v) { return { v.x, v.y, v.z }; }
static json SerializeVec(const Vec4& v) { return { v.x, v.y, v.z, v.w }; }

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

			Log(Success, "EditorService: Received message: ", msg->str.c_str());

			try
			{
				auto l_json = json::parse(msg->str);
				if (!l_json.contains("type"))
				{
					Log(Warning, "EditorService: message missing 'type' field; dropping.");
					return;
				}
				std::string l_type = l_json["type"];

				EditorServiceImpl::Handler l_handler;
				{
					std::lock_guard<std::mutex> lock(m_Impl->mutex);
					auto it = m_Impl->handlers.find(l_type);
					if (it != m_Impl->handlers.end())
						l_handler = it->second;
				}
				if (l_handler)
					l_handler(l_json, webSocket);
				else
					Log(Warning, "EditorService: no handler registered for message type: ", l_type.c_str());
			}
			catch (const std::exception& e)
			{
				Log(Error, "EditorService: Failed to parse JSON: ", e.what());
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

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "EditorService: WebSocket server started on port 8081.");
	return true;
}

void EditorService::RegisterBuiltinHandlers()
{
	auto reg = [this](const char* type, EditorServiceImpl::Handler h) {
		std::lock_guard<std::mutex> lock(m_Impl->mutex);
		m_Impl->handlers[type] = std::move(h);
	};

	reg("HELO", [this](const json& msg, ix::WebSocket& ws) {
		if (msg.contains("pid"))
		{
			m_clientPID = msg["pid"].get<uint32_t>();
			Log(Success, "EditorService: Client PID registered: ", m_clientPID);
		}

		void* l_sharedHandle = g_Engine->Get<FrameManagementService>()->GetViewportSharedHandle();
		auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

		if (l_sharedHandle && m_clientPID > 0)
		{
#ifdef INNO_PLATFORM_WIN
			HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, m_clientPID);
			if (hProcess)
			{
				HANDLE duplicateHandle = NULL;
				if (DuplicateHandle(GetCurrentProcess(), l_sharedHandle, hProcess, &duplicateHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
				{
					l_sharedHandle = duplicateHandle;
					Log(Success, "EditorService: Duplicated shared handle for UI process (PID ", m_clientPID, "): ", (uint64_t)l_sharedHandle);
				}
				else
				{
					Log(Error, "EditorService: Failed to duplicate handle. GetLastError=", (uint64_t)GetLastError());
				}
				CloseHandle(hProcess);
			}
			else
			{
				Log(Error, "EditorService: Failed to open UI process (PID ", m_clientPID, ") for handle duplication. GetLastError=", (uint64_t)GetLastError());
			}
#endif
		}

		uint32_t l_width  = l_resolution.x ? l_resolution.x : 1280;
		uint32_t l_height = l_resolution.y ? l_resolution.y : 720;

		json l_reply;
		l_reply["type"]         = "HELLO_REPLY";
		l_reply["sharedHandle"] = (uint64_t)l_sharedHandle;
		l_reply["width"]        = l_width;
		l_reply["height"]       = l_height;
		l_reply["format"]       = "rgba";
		ws.send(l_reply.dump());
		Log(Success, "EditorService: Sent HELLO_REPLY with sharedHandle: ", (uint64_t)l_sharedHandle, " size: ", l_width, "x", l_height);
	});

	reg("GET_SCENE", [](const json& /*msg*/, ix::WebSocket& ws) {
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
		json l_reply;
		l_reply["type"]     = "SCENE_DATA";
		l_reply["entities"] = l_entities;
		ws.send(l_reply.dump());
	});

	reg("GET_ENTITY_DETAILS", [](const json& msg, ix::WebSocket& ws) {
		if (!msg.contains("id"))
			return;
		EntityID l_id = (EntityID)msg["id"].get<uint32_t>();
		auto l_registry = g_Engine->Get<EntityRegistry>();
		if (!l_registry->IsValid(l_id))
			return;

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
			l_comp["type"]      = "LightComponent";
			l_comp["color"]     = SerializeVec(l_light->m_RGBColor);
			l_comp["shape"]     = SerializeVec(l_light->m_Shape);
			l_comp["lightType"] = (int)l_light->m_LightType;
			l_comp["intensity"] = l_light->m_LuminousFlux;
			l_components.push_back(l_comp);
		}

		l_details["components"] = l_components;

		json l_reply;
		l_reply["type"]    = "ENTITY_DETAILS";
		l_reply["details"] = l_details;
		ws.send(l_reply.dump());
	});

	reg("LIST_DEV_TOGGLES", [](const json& /*msg*/, ix::WebSocket& ws) {
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

		json l_reply;
		l_reply["type"]    = "DEV_TOGGLES";
		l_reply["toggles"] = l_toggles;
		l_reply["actions"] = l_actions;
		ws.send(l_reply.dump());
	});

	reg("SET_DEV_TOGGLE", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("name") || !msg.contains("value"))
			return;
		std::string l_name  = msg["name"];
		bool        l_value = msg["value"];
		if (!DevToggleRegistry::Set(l_name, l_value))
			Log(Warning, "EditorService: SET_DEV_TOGGLE for unknown toggle: ", l_name.c_str());
	});

	reg("TRIGGER_DEV_ACTION", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("name"))
			return;
		std::string l_name = msg["name"];
		if (!DevToggleRegistry::Trigger(l_name))
			Log(Warning, "EditorService: TRIGGER_DEV_ACTION for unknown action: ", l_name.c_str());
	});

	reg("LIST_TASKS", [](const json& /*msg*/, ix::WebSocket& ws) {
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
		json l_reply;
		l_reply["type"]    = "TASK_GRAPH";
		l_reply["threads"] = l_threads;
		ws.send(l_reply.dump());
	});

	reg("LIST_RENDER_TARGETS", [](const json& /*msg*/, ix::WebSocket& ws) {
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

		json l_reply;
		l_reply["type"]   = "RENDER_TARGETS";
		l_reply["passes"] = l_passes;
		auto l_current = ViewportSourceOverride::Get();
		if (l_current.has_value())
		{
			json l_sel;
			l_sel["pass"]       = l_current->m_PassName;
			l_sel["rtIndex"]    = l_current->m_RTIndex;
			l_reply["override"] = l_sel;
		}
		ws.send(l_reply.dump());
	});

	reg("SET_VIEWPORT_SOURCE", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (msg.contains("pass") && msg.contains("rtIndex"))
		{
			std::string l_pass    = msg["pass"];
			uint32_t    l_rtIndex = msg["rtIndex"].get<uint32_t>();
			ViewportSourceOverride::Set(l_pass, l_rtIndex);
		}
		else
		{
			ViewportSourceOverride::Reset();
		}
	});

	reg("LOAD_SCENE", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("path"))
			return;
		std::string l_path = msg["path"];
		Log(Success, "EditorService: Requesting scene load: ", l_path.c_str());
		// AsyncLoad=true: this callback runs on the WebSocket worker
		// thread; SceneService::Load from a non-main thread must be
		// async or it races DX12 resource lifecycle on the render thread.
		g_Engine->Get<SceneService>()->Load(l_path.c_str(), true);
	});

	reg("SAVE_SCENE", [](const json& /*msg*/, ix::WebSocket& /*ws*/) {
		Log(Success, "EditorService: Requesting scene save.");
		auto sceneService = g_Engine->Get<SceneService>();
		sceneService->Save(sceneService->GetCurrentSceneName().c_str());
	});

	reg("IMPORT_ASSET", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("path"))
			return;
		std::string l_path = msg["path"];
		Log(Success, "EditorService: Requesting asset import: ", l_path.c_str());
		g_Engine->Get<AssetService>()->Import(l_path.c_str());
	});

	reg("ENTITY_CREATE", [](const json& msg, ix::WebSocket& ws) {
		std::string l_name = msg.value("name", std::string("Entity"));
		auto l_id = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Scene, l_name.c_str());
		json l_reply;
		l_reply["type"] = "ENTITY_CREATED";
		l_reply["id"]   = (uint32_t)l_id;
		l_reply["name"] = l_name;
		ws.send(l_reply.dump());
	});

	reg("ENTITY_DELETE", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("id"))
			return;
		auto* l_registry = g_Engine->Get<EntityRegistry>();
		EntityID l_id = (EntityID)msg["id"].get<uint32_t>();
		if (!l_registry->IsValid(l_id))
		{
			Log(Warning, "EditorService: ENTITY_DELETE for invalid id ", (uint32_t)l_id);
			return;
		}
		// Refuse to delete persistent (engine-owned) entities; the editor
		// only owns scene-bound entities. Otherwise a stray click could
		// nuke the player camera, sun, etc.
		if (l_registry->GetLifespan(l_id) != ObjectLifespan::Scene)
		{
			Log(Warning, "EditorService: refusing ENTITY_DELETE on non-scene entity ", (uint32_t)l_id);
			return;
		}
		l_registry->Destroy(l_id);
	});

	reg("ENTITY_RENAME", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("id") || !msg.contains("name"))
			return;
		auto* l_registry = g_Engine->Get<EntityRegistry>();
		EntityID l_id = (EntityID)msg["id"].get<uint32_t>();
		std::string l_name = msg["name"];
		if (!l_registry->Rename(l_id, l_name.c_str()))
			Log(Warning, "EditorService: ENTITY_RENAME failed for id ", (uint32_t)l_id);
	});

	reg("UPDATE_ENTITY_PROPERTY", [](const json& msg, ix::WebSocket& /*ws*/) {
		if (!msg.contains("id") || !msg.contains("component") || !msg.contains("property") || !msg.contains("value"))
			return;
		EntityID    l_id       = (EntityID)msg["id"].get<uint32_t>();
		std::string l_compType = msg["component"];
		std::string l_prop     = msg["property"];
		auto        l_val      = msg["value"];

		auto l_registry = g_Engine->Get<EntityRegistry>();
		if (!l_registry->IsValid(l_id))
			return;

		if (l_compType == "TransformComponent")
		{
			auto l_transform = l_registry->Get<TransformComponent>(l_id);
			if (!l_transform)
				return;
			if (l_prop == "pos")        l_transform->m_LocalPos   = Vec3(l_val[0], l_val[1], l_val[2]);
			else if (l_prop == "scale") l_transform->m_LocalScale = Vec3(l_val[0], l_val[1], l_val[2]);
			// TODO: Rotation (Quat)
		}
		else if (l_compType == "LightComponent")
		{
			auto l_light = l_registry->Get<LightComponent>(l_id);
			if (!l_light)
				return;
			if (l_prop == "intensity")   l_light->m_LuminousFlux = l_val.get<float>();
			else if (l_prop == "color")  l_light->m_RGBColor     = Vec4(l_val[0], l_val[1], l_val[2], 1.0f);
		}
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

void EditorService::NotifyViewportReady(void* sharedHandle)
{
	if (!m_Server)
		return;

	auto l_server = GetServer(m_Server);
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	uint32_t l_width  = l_resolution.x ? l_resolution.x : 1280;
	uint32_t l_height = l_resolution.y ? l_resolution.y : 720;

	void* l_handleToSend = sharedHandle;

	if (l_handleToSend && m_clientPID > 0)
	{
#ifdef INNO_PLATFORM_WIN
		HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, m_clientPID);
		if (hProcess)
		{
			HANDLE duplicateHandle = NULL;
			if (DuplicateHandle(GetCurrentProcess(), l_handleToSend, hProcess, &duplicateHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
			{
				l_handleToSend = duplicateHandle;
				Log(Success, "EditorService: Duplicated shared handle for UI process (PID ", m_clientPID, ") in NotifyViewportReady: ", (uint64_t)l_handleToSend);
			}
			else
			{
				Log(Error, "EditorService: Failed to duplicate handle in NotifyViewportReady. GetLastError=", (uint64_t)GetLastError());
			}
			CloseHandle(hProcess);
		}
		else
		{
			Log(Error, "EditorService: Failed to open UI process (PID ", m_clientPID, ") in NotifyViewportReady for handle duplication. GetLastError=", (uint64_t)GetLastError());
		}
#endif
	}

	json l_reply;
	l_reply["type"]         = "VIEWPORT_READY";
	l_reply["sharedHandle"] = (uint64_t)l_handleToSend;
	l_reply["width"]        = l_width;
	l_reply["height"]       = l_height;
	l_reply["format"]       = "rgba";

	auto l_msg = l_reply.dump();
	for (auto&& client : l_server->getClients())
	{
		client->send(l_msg);
	}
	Log(Success, "EditorService: Broadcasted VIEWPORT_READY with handle: ", (uint64_t)l_handleToSend);
}
