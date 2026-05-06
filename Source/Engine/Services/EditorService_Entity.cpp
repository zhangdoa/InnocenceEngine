#include "EditorService_Internal.h"
#include "../Engine.h"
#include "EntityRegistry.h"
#include "../Component/TransformComponent.h"
#include "../Component/LightComponent.h"

using namespace Inno;

void EditorService::RegisterEntityHandlers()
{
	auto reg = [this](const char* type, EditorServiceImpl::Handler h) {
		std::lock_guard<std::mutex> lock(m_Impl->mutex);
		m_Impl->handlers[type] = std::move(h);
	};

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
				// An explicit color edit is the user signal to leave K-mode;
				// otherwise LightSimulationService::Update() overwrites the
				// commit with ColorTemperatureToRGB(m_ColorTemperature) on
				// the next frame and the inspector readback drifts back to
				// the K-derived value. The reverse path — back into K-mode —
				// is the explicit useColorTemperature setter below.
				l_light->m_RGBColor = Vec4(l_val[0], l_val[1], l_val[2], 1.0f);
				l_light->m_UseColorTemperature = false;
				committed = SerializeVec(l_light->m_RGBColor);
			}
			else if (l_prop == "castShadow")
			{
				l_light->m_CastShadow = l_val.get<bool>();
				committed = l_light->m_CastShadow;
			}
			else if (l_prop == "useColorTemperature")
			{
				l_light->m_UseColorTemperature = l_val.get<bool>();
				committed = l_light->m_UseColorTemperature;
			}
			else if (l_prop == "colorTemperature")
			{
				l_light->m_ColorTemperature = l_val.get<float>();
				committed = l_light->m_ColorTemperature;
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
