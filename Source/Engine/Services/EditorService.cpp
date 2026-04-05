#include "EditorService.h"
#include "../Engine.h"
#include "../Common/LogService.h"
#include "EntityRegistry.h"
#include "FrameManagementService.h"
#include "../ThirdParty/JSONWrapper/JSONWrapper.h"

#include <ixwebsocket/IXWebSocketServer.h>
#include <ixwebsocket/IXWebSocket.h>

using namespace Inno;

EditorService::EditorService() = default;
EditorService::~EditorService() = default;

bool EditorService::Setup(IServiceConfig* config)
{
	m_ObjectStatus = ObjectStatus::Created;
	Log(Success, "EditorService: Setup finished.");
	return true;
}

bool EditorService::Initialize()
{
	m_Server = std::make_unique<ix::WebSocketServer>(8081);

	m_Server->setOnClientMessageCallback([this](std::shared_ptr<ix::ConnectionState> connectionState, ix::WebSocket& webSocket, const ix::WebSocketMessagePtr& msg)
		{
			if (msg->type == ix::WebSocketMessageType::Message)
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
							void* l_sharedHandle = g_Engine->Get<FrameManagementService>()->GetViewportSharedHandle();
							auto l_renderPass = g_Engine->Get<FrameManagementService>()->GetSwapChainRenderPassComponent();

							uint32_t l_width = 1280;
							uint32_t l_height = 720;

							if (l_renderPass)
							{
								l_width = l_renderPass->m_RenderPassDesc.m_RenderTargetDesc.Width;
								l_height = l_renderPass->m_RenderPassDesc.m_RenderTargetDesc.Height;
							}

							json l_reply;
							l_reply["type"] = "HELLO_REPLY";
							l_reply["sharedHandle"] = (uint64_t)l_sharedHandle;
							l_reply["width"] = l_width;
							l_reply["height"] = l_height;
							l_reply["format"] = "rgba"; // Assume RGBA for now

							webSocket.send(l_reply.dump());
							Log(Success, "EditorService: Sent HELLO_REPLY with sharedHandle: ", l_sharedHandle, " size: ", l_width, "x", l_height);
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
									
									// Placeholder for components
									l_details["components"] = json::array();
									
									json l_reply;
									l_reply["type"] = "ENTITY_DETAILS";
									l_reply["details"] = l_details;
									webSocket.send(l_reply.dump());
								}
							}
						}					}
				}
				catch (const std::exception& e)
				{
					Log(Error, "EditorService: Failed to parse JSON: ", e.what());
				}
			}
		});

	auto l_res = m_Server->listen();
	if (!l_res.first)
	{
		Log(Error, "EditorService: Failed to start WebSocket server: ", l_res.second.c_str());
		return false;
	}

	m_Server->start();

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
		m_Server->stop();
	}
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "EditorService: Terminated.");
	return true;
}

ObjectStatus EditorService::GetStatus()
{
	return m_ObjectStatus;
}
