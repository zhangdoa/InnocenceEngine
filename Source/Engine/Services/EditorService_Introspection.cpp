#include "EditorService_Internal.h"
#include "../Engine.h"
#include "EntityRegistry.h"
#include "DevToggleRegistry.h"
#include "RenderPassResourceService.h"
#include "ViewportSourceOverride.h"
#include "../Common/TaskScheduler.h"
#include "../Component/TransformComponent.h"
#include "../Component/LightComponent.h"

using namespace Inno;

void EditorService::RegisterIntrospectionHandlers()
{
	auto reg = [this](const char* type, EditorServiceImpl::Handler h) {
		std::lock_guard<std::mutex> lock(m_Impl->mutex);
		m_Impl->handlers[type] = std::move(h);
	};

	reg("HELLO", [](const json& /*payload*/, ix::WebSocket& /*ws*/) -> json {
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
			l_comp["type"]                = "LightComponent";
			l_comp["color"]               = SerializeVec(l_light->m_RGBColor);
			l_comp["shape"]               = SerializeVec(l_light->m_Shape);
			l_comp["lightType"]           = (int)l_light->m_LightType;
			l_comp["intensity"]           = l_light->m_LuminousFlux;
			l_comp["castShadow"]          = l_light->m_CastShadow;
			l_comp["useColorTemperature"] = l_light->m_UseColorTemperature;
			l_comp["colorTemperature"]    = l_light->m_ColorTemperature;
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
}
