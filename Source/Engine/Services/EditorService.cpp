#include "EditorService.h"
#include "../Engine.h"
#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "AssetService.h"
#include "RenderingConfigurationService.h"
#include "FrameManagementService.h"
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

using namespace Inno;

// Helper to cast opaque pointer to server
inline ix::WebSocketServer* GetServer(void* ptr) { return static_cast<ix::WebSocketServer*>(ptr); }

// Helper to serialize Vec3/Vec4
static json SerializeVec(const Vec3& v) { return { v.x, v.y, v.z }; }
static json SerializeVec(const Vec4& v) { return { v.x, v.y, v.z, v.w }; }

bool EditorService::Setup(IServiceConfig* config)
{
	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "EditorService: Setup finished.");
	return true;
}

bool EditorService::Initialize()
{
	auto l_server = new ix::WebSocketServer(8081, "127.0.0.1");
	m_Server = l_server;

	l_server->setOnClientMessageCallback([this, l_server](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
		{
			if (msg->type == ix::WebSocketMessageType::Open)
			{
				Log(Success, "EditorService: New client connection opened.");
			}
			else if (msg->type == ix::WebSocketMessageType::Close)
			{
				Log(Success, "EditorService: Client connection closed.");
			}
			else if (msg->type == ix::WebSocketMessageType::Error)
			{
				Log(Error, "EditorService: WebSocket error: ", msg->errorInfo.reason.c_str());
			}
			else if (msg->type == ix::WebSocketMessageType::Message)
			{
				Log(Success, "EditorService: Received message: ", msg->str.c_str());
				
				try
				{
					auto l_json = json::parse(msg->str);
					if (l_json.contains("type"))
					{
						std::string l_type = l_json["type"];
						if (l_type == "HELO")
						{
							if (l_json.contains("pid"))
							{
								m_clientPID = l_json["pid"].get<uint32_t>();
								Log(Success, "EditorService: Client PID registered: ", m_clientPID);
							}

							void* l_sharedHandle = g_Engine->Get<FrameManagementService>()->GetViewportSharedHandle();
							auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

							// Duplicate the handle for the client if we have a PID and a handle
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

							uint32_t l_width = l_resolution.x;
							uint32_t l_height = l_resolution.y;

							if (l_width == 0 || l_height == 0)
							{
								l_width = 1280;
								l_height = 720;
							}

							json l_reply;
							l_reply["type"] = "HELLO_REPLY";
							l_reply["sharedHandle"] = (uint64_t)l_sharedHandle;
							l_reply["width"] = l_width;
							l_reply["height"] = l_height;
							l_reply["format"] = "rgba"; // Assume RGBA for now

							webSocket.send(l_reply.dump());
							Log(Success, "EditorService: Sent HELLO_REPLY with sharedHandle: ", (uint64_t)l_sharedHandle, " size: ", l_width, "x", l_height);
						}

						else if (l_type == "GET_SCENE")
						{
							auto l_registry = g_Engine->Get<EntityRegistry>();
							auto l_ids = l_registry->GetAllEntityIDs(ObjectLifespan::Scene);
							
							json l_entities = json::array();
							for (auto l_id : l_ids)
							{
								json l_entity;
								l_entity["id"] = (uint32_t)l_id;
								l_entity["name"] = l_registry->GetName(l_id);
								l_entities.push_back(l_entity);
							}

							json l_reply;
							l_reply["type"] = "SCENE_DATA";
							l_reply["entities"] = l_entities;
							webSocket.send(l_reply.dump());
						}
						else if (l_type == "GET_ENTITY_DETAILS")
						{
							if (l_json.contains("id"))
							{
								EntityID l_id = (EntityID)l_json["id"].get<uint32_t>();
								auto l_registry = g_Engine->Get<EntityRegistry>();
								
								if (l_registry->IsValid(l_id))
								{
									json l_details;
									l_details["id"] = (uint32_t)l_id;
									l_details["name"] = l_registry->GetName(l_id);
									
									json l_components = json::array();
									
									// Transform
									auto l_transform = l_registry->Get<TransformComponent>(l_id);
									if (l_transform)
									{
										json l_comp;
										l_comp["type"] = "TransformComponent";
										l_comp["pos"] = SerializeVec(l_transform->m_LocalPos);
										l_comp["rot"] = SerializeVec(l_transform->m_LocalRot);
										l_comp["scale"] = SerializeVec(l_transform->m_LocalScale);
										l_components.push_back(l_comp);
									}

									// Light
									auto l_light = l_registry->Get<LightComponent>(l_id);
									if (l_light)
									{
										json l_comp;
										l_comp["type"] = "LightComponent";
										l_comp["color"] = SerializeVec(l_light->m_RGBColor);
										l_comp["shape"] = SerializeVec(l_light->m_Shape);
										l_comp["lightType"] = (int)l_light->m_LightType;
										l_comp["intensity"] = l_light->m_LuminousFlux;
										l_components.push_back(l_comp);
									}
									
									l_details["components"] = l_components;
									
									json l_reply;
									l_reply["type"] = "ENTITY_DETAILS";
									l_reply["details"] = l_details;
									webSocket.send(l_reply.dump());
								}
							}
						}
						else if (l_type == "LOAD_SCENE")
						{
							if (l_json.contains("path"))
							{
								std::string l_path = l_json["path"];
								Log(Success, "EditorService: Requesting scene load: ", l_path.c_str());
								g_Engine->Get<SceneService>()->Load(l_path.c_str());
							}
						}
						else if (l_type == "SAVE_SCENE")
						{
							Log(Success, "EditorService: Requesting scene save.");
							auto sceneService = g_Engine->Get<SceneService>();
							sceneService->Save(sceneService->GetCurrentSceneName().c_str());
						}
						else if (l_type == "IMPORT_ASSET")
						{
							if (l_json.contains("path"))
							{
								std::string l_path = l_json["path"];
								Log(Success, "EditorService: Requesting asset import: ", l_path.c_str());

								g_Engine->Get<AssetService>()->Import(l_path.c_str());
							}
						}						else if (l_type == "UPDATE_ENTITY_PROPERTY")
						{
							if (l_json.contains("id") && l_json.contains("component") && l_json.contains("property") && l_json.contains("value"))
							{
								EntityID l_id = (EntityID)l_json["id"].get<uint32_t>();
								std::string l_compType = l_json["component"];
								std::string l_prop = l_json["property"];
								auto l_val = l_json["value"];

								auto l_registry = g_Engine->Get<EntityRegistry>();
								if (l_registry->IsValid(l_id))
								{
									if (l_compType == "TransformComponent")
									{
										auto l_transform = l_registry->Get<TransformComponent>(l_id);
										if (l_transform)
										{
											if (l_prop == "pos") { l_transform->m_LocalPos = Vec3(l_val[0], l_val[1], l_val[2]); }
											else if (l_prop == "scale") { l_transform->m_LocalScale = Vec3(l_val[0], l_val[1], l_val[2]); }
											// TODO: Rotation (Quat)
										}
									}
									else if (l_compType == "LightComponent")
									{
										auto l_light = l_registry->Get<LightComponent>(l_id);
										if (l_light)
										{
											if (l_prop == "intensity") { l_light->m_LuminousFlux = l_val.get<float>(); }
											else if (l_prop == "color") { l_light->m_RGBColor = Vec4(l_val[0], l_val[1], l_val[2], 1.0f); }
										}
									}
								}
							}
						}
					}
				}
				catch (const std::exception& e)
				{
					Log(Error, "EditorService: Failed to parse JSON: ", e.what());
				}
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
	if (m_Server)
	{
		auto l_server = GetServer(m_Server);
		auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		uint32_t l_width = l_resolution.x;
		uint32_t l_height = l_resolution.y;

		if (l_width == 0 || l_height == 0)
		{
			l_width = 1280;
			l_height = 720;
		}

		void* l_handleToSend = sharedHandle;

		// Duplicate the handle for the client if we have a PID
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
		l_reply["type"] = "VIEWPORT_READY";
		l_reply["sharedHandle"] = (uint64_t)l_handleToSend;
		l_reply["width"] = l_width;
		l_reply["height"] = l_height;
		l_reply["format"] = "rgba";

		auto l_msg = l_reply.dump();
		for (auto&& client : l_server->getClients())
		{
			client->send(l_msg);
		}
		Log(Success, "EditorService: Broadcasted VIEWPORT_READY with handle: ", (uint64_t)l_handleToSend);
	}
}
