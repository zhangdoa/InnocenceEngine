#include "EditorService.h"
#include "../Engine.h"
#include "../Common/LogService.h"
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
							
							json l_reply;
							l_reply["type"] = "HELLO_REPLY";
							l_reply["sharedHandle"] = (uint64_t)l_sharedHandle;
							
							webSocket.send(l_reply.dump());
							Log(Success, "EditorService: Sent HELLO_REPLY with sharedHandle: ", l_sharedHandle);
						}
					}
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
