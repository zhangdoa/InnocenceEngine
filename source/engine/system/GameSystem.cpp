#include "GameSystem.h"
#include "../common/config.h"
#include "../component/GameSystemComponent.h"

#include <fstream>
#include <iomanip>

#include "json/json.hpp"
using json = nlohmann::json;

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoGameSystemNS
{
	bool setup();
	bool loadScene();
	bool saveScene();

	std::string getEntityName(EntityID entityID);

	void to_json(json& j, const enitityNamePair& p);
	void to_json(json& j, const TransformComponent& p);
	void to_json(json& j, const TransformVector& p);

	void to_json(json& j, const VisibleComponent& p);
	void to_json(json& j, const CameraComponent& p);

	void to_json(json& j, const vec4& p);
	void to_json(json& j, const DirectionalLightComponent& p);
	void to_json(json& j, const PointLightComponent& p);
	void to_json(json& j, const SphereLightComponent& p);

	template<typename T>
	bool saveComponentData(json& topLevel, T* rhs);

#define saveComponentDataDefi( className ) \
inline bool saveComponentData<className>(json& topLevel, className* rhs) \
{ \
		json j; \
		to_json(j, *rhs); \
 \
		auto result = std::find_if( \
			topLevel["SceneEntities"].begin(), \
			topLevel["SceneEntities"].end(), \
			[&](auto& val) -> bool { \
			return val["EntityID"] == rhs->m_parentEntity; \
		}); \
 \
		if (result != topLevel["SceneEntities"].end()) \
		{ \
			result.value()["ChildrenComponents"].emplace_back(j); \
			return true; \
		} \
		else \
		{ \
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GameSystem: Entity ID " + std::to_string(rhs->m_parentEntity) + " is invalid when saving " + std::string(#className) + "."); \
			return false; \
		} \
}
	template<>
	saveComponentDataDefi(TransformComponent);
	template<>
	saveComponentDataDefi(VisibleComponent);
	template<>
	saveComponentDataDefi(CameraComponent);
	template<>
	saveComponentDataDefi(DirectionalLightComponent);
	template<>
	saveComponentDataDefi(PointLightComponent);
	template<>
	saveComponentDataDefi(SphereLightComponent);

	void sortTransformComponentsVector();

	void updateTransform();

	EntityID createEntity(const std::string & entityName);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	static GameSystemComponent* g_GameSystemComponent;
	IGameInstance* m_gameInstance;
}

void InnoGameSystemNS::to_json(json& j, const enitityNamePair& p)
{
	j = json{
		{"EntityID", p.first},
	{"EntityName", p.second},
	};
}

void InnoGameSystemNS::to_json(json& j, const TransformVector& p)
{
	j = json
	{
		{
			"Position",
			{
				{
					"X", p.m_pos.x
				},
				{
					"Y", p.m_pos.y
				},
				{
					"Z", p.m_pos.z
				}
			}		
		},
		{
			"Rotation",
			{
				{
					"X", p.m_rot.x
				},
				{
					"Y", p.m_rot.y
				},
				{
					"Z", p.m_rot.z
				},
				{
					"W", p.m_rot.w
				}
			}		
		}
	};
}

void InnoGameSystemNS::to_json(json& j, const vec4& p)
{
	j = json
	{
		{
				"R", p.x
		},
		{
				"G", p.y
		},
		{
				"B", p.z
		},
		{
				"A", p.w
		}
	};
}

void InnoGameSystemNS::to_json(json& j, const TransformComponent& p)
{
	json localTransformVector;

	to_json(localTransformVector, p.m_localTransformVector);

	auto parentTransformComponentEntityName = getEntityName(p.m_parentTransformComponent->m_parentEntity);

	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<TransformComponent>()},
		{"ParentTransformComponentEntityName", parentTransformComponentEntityName},
		{"LocalTransformVector",
			localTransformVector
		},
	};
}

void InnoGameSystemNS::to_json(json& j, const VisibleComponent& p)
{
	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<VisibleComponent>()},
		{"VisiblilityType", p.m_visiblilityType},
		{"MeshShapeType", p.m_meshShapeType},
		{"MeshPrimitiveTopology", p.m_meshDrawMethod},
		{"TextureWrapMethod", p.m_textureWrapMethod},
		{"drawAABB", p.m_drawAABB},
		{"ModelFileName", p.m_modelFileName},
	};
}

void InnoGameSystemNS::to_json(json& j, const CameraComponent& p)
{
	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<CameraComponent>()},
		{"FOVX", p.m_FOVX},
		{"WHRatio", p.m_WHRatio},
		{"ZNear", p.m_zNear},
		{"ZFar", p.m_zFar},
	};
}

void InnoGameSystemNS::to_json(json& j, const DirectionalLightComponent& p)
{
	json color;
	to_json(color, p.m_color);

	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<DirectionalLightComponent>()},
		{"LuminousFlux", p.m_luminousFlux},
		{"Color", color},
		{"drawAABB", p.m_drawAABB},
	};
}

void InnoGameSystemNS::to_json(json& j, const PointLightComponent& p)
{
	json color;
	to_json(color, p.m_color);

	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<PointLightComponent>()},
		{"LuminousFlux", p.m_luminousFlux},
		{"Color", color},
	};
}

void InnoGameSystemNS::to_json(json& j, const SphereLightComponent& p)
{
	json color;
	to_json(color, p.m_color);

	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<SphereLightComponent>()},
		{"SphereRadius", p.m_sphereRadius},
		{"LuminousFlux", p.m_luminousFlux},
		{"Color", color},
	};
}

bool InnoGameSystemNS::loadScene()
{
	return true;
}

bool InnoGameSystemNS::saveScene()
{
	std::ofstream o;
	o.open("..//res//scenes//test.InnoScene", std::ios::out | std::ios::trunc);

	json topLevel;
	topLevel["SceneName"] = "testSave";

	// save entities name and ID
	for (auto& i : g_GameSystemComponent->m_enitityNameMap)
	{
		json j;
		to_json(j, i);
		topLevel["SceneEntities"].emplace_back(j);
	}

	// save childern components
	for (auto i : g_GameSystemComponent->m_TransformComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_GameSystemComponent->m_VisibleComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_GameSystemComponent->m_CameraComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_GameSystemComponent->m_DirectionalLightComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_GameSystemComponent->m_PointLightComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_GameSystemComponent->m_SphereLightComponents)
	{
		saveComponentData(topLevel, i);
	}

	o << std::setw(4) << topLevel << std::endl;
	o.close();

	return true;
}

std::string InnoGameSystemNS::getEntityName(EntityID entityID)
{
	auto result = std::find_if(
		InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.begin(),
		InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.end(),
		[&](auto& val)-> bool {
		return val.first == entityID;
	}
	);

	if (result == InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: can't find entity name by ID " + std::to_string(entityID) + " !");
		return "AbnormalEntity";
	}

	return result->second;
}

bool InnoGameSystemNS::setup()
{
	g_GameSystemComponent = &GameSystemComponent::get();

	// setup root TransformComponent
	g_GameSystemComponent->m_rootTransformComponent = new TransformComponent();
	g_GameSystemComponent->m_rootTransformComponent->m_parentTransformComponent = nullptr;

	g_GameSystemComponent->m_rootTransformComponent->m_parentEntity = createEntity("RootTransform");

	g_GameSystemComponent->m_rootTransformComponent->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(g_GameSystemComponent->m_rootTransformComponent->m_localTransformVector);
	g_GameSystemComponent->m_rootTransformComponent->m_globalTransformVector = g_GameSystemComponent->m_rootTransformComponent->m_localTransformVector;
	g_GameSystemComponent->m_rootTransformComponent->m_globalTransformMatrix = g_GameSystemComponent->m_rootTransformComponent->m_localTransformMatrix;

	m_objectStatus = ObjectStatus::ALIVE;

	if (!m_gameInstance->setup())
	{
		return false;
	}

	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::setup()
{
	if (!InnoGameSystemNS::setup())
	{
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameInstance setup finished.");

	return true;
}

void InnoGameSystemNS::sortTransformComponentsVector()
{
	//construct the hierarchy tree
	for (auto i : InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents)
	{
		if (i->m_parentTransformComponent)
		{
			i->m_transformHierarchyLevel = i->m_parentTransformComponent->m_transformHierarchyLevel + 1;
		}
	}
	//from top to bottom
	std::sort(g_GameSystemComponent->m_TransformComponents.begin(), g_GameSystemComponent->m_TransformComponents.end(), [&](TransformComponent* a, TransformComponent* b)
	{
		return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
	});
}

void InnoGameSystemNS::updateTransform()
{
	std::for_each(InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.begin(), InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_localTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_localTransformVector);
		val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
		val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
	});
}

// @TODO: add a cache function for after-rendering business
INNO_SYSTEM_EXPORT void InnoGameSystem::saveComponentsCapture()
{
	std::for_each(InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.begin(), InnoGameSystemNS::g_GameSystemComponent->m_TransformComponents.end(), [&](TransformComponent* val)
	{
		val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
	});
}

INNO_SYSTEM_EXPORT void InnoGameSystem::setGameInstance(IGameInstance * rhs)
{
	InnoGameSystemNS::m_gameInstance = rhs;
}

EntityID InnoGameSystemNS::createEntity(const std::string & entityName)
{
	auto result = std::find_if(
		InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.begin(),
		InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.end(),
		[&](auto& val) -> bool {
		return val.second == entityName;
	});

	if (result != InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: duplicated entity name!");
		return 0;
	}

	auto l_entityID = InnoMath::createEntityID();
	InnoGameSystemNS::g_GameSystemComponent->m_enitityNameMap.emplace(l_entityID, entityName);
	return l_entityID;
}


INNO_SYSTEM_EXPORT EntityID InnoGameSystem::createEntity(const std::string & entityName)
{
	return InnoGameSystemNS::createEntity(entityName);
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::initialize()
{
	InnoGameSystemNS::sortTransformComponentsVector();
	InnoGameSystemNS::updateTransform();

	if (!InnoGameSystemNS::m_gameInstance->initialize())
	{
		return false;
	}

	InnoGameSystemNS::saveScene();
	InnoGameSystemNS::loadScene();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::update()
{
	auto temp = g_pCoreSystem->getTaskSystem()->submit([]()
	{
		InnoGameSystemNS::updateTransform();
	});
	InnoGameSystemNS::g_GameSystemComponent->m_asyncTask = &temp;

	if (!GameSystemComponent::get().m_pauseGameUpdate)
	{
		if (!InnoGameSystemNS::m_gameInstance->update())
		{
			return false;
		}
	}

	return true;
}

INNO_SYSTEM_EXPORT bool InnoGameSystem::terminate()
{
	if (!InnoGameSystemNS::m_gameInstance->terminate())
	{
		return false;
	}

	delete InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent;

	InnoGameSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been terminated.");
	return true;
}

#define spawnComponentImplDefi( className ) \
INNO_SYSTEM_EXPORT void InnoGameSystem::spawnComponent(className* rhs, EntityID parentEntity) \
{ \
	rhs->m_parentEntity = parentEntity; \
	InnoGameSystemNS::g_GameSystemComponent->m_##className##s.emplace_back(rhs); \
	InnoGameSystemNS::g_GameSystemComponent->m_##className##sMap.emplace(parentEntity, rhs); \
\
	auto indexOfTheComponent = InnoGameSystemNS::g_GameSystemComponent->m_##className##s.size(); \
	auto l_componentName = std::string(#className) + "_" + std::to_string(indexOfTheComponent); \
	auto l_componentType = InnoUtility::getComponentType<className>(); \
	auto l_componentMetaDataPair = componentMetadataPair(l_componentType, l_componentName); \
\
	auto& result = InnoGameSystemNS::g_GameSystemComponent->m_enitityChildrenComponentsMetadataMap.find(parentEntity); \
	if (result != InnoGameSystemNS::g_GameSystemComponent->m_enitityChildrenComponentsMetadataMap.end()) \
	{ \
		auto l_componentMetadataMap = &result->second; \
		l_componentMetadataMap->emplace(rhs, l_componentMetaDataPair); \
	} \
	else \
	{ \
		auto l_componentMetadataMap = componentMetadataMap(); \
		l_componentMetadataMap.emplace(rhs, l_componentMetaDataPair); \
		InnoGameSystemNS::g_GameSystemComponent->m_enitityChildrenComponentsMetadataMap.emplace(parentEntity, std::move(l_componentMetadataMap)); \
	} \
}

spawnComponentImplDefi(TransformComponent)
spawnComponentImplDefi(VisibleComponent)
spawnComponentImplDefi(DirectionalLightComponent)
spawnComponentImplDefi(PointLightComponent)
spawnComponentImplDefi(SphereLightComponent)
spawnComponentImplDefi(CameraComponent)
spawnComponentImplDefi(InputComponent)
spawnComponentImplDefi(EnvironmentCaptureComponent)

INNO_SYSTEM_EXPORT std::string InnoGameSystem::getGameName()
{
	return std::string("GameInstance");
}

// @TODO: return multiple instances
#define getComponentImplDefi( className ) \
INNO_SYSTEM_EXPORT className* InnoGameSystem::get##className(EntityID parentEntity) \
{ \
	auto result = InnoGameSystemNS::g_GameSystemComponent->m_##className##sMap.find(parentEntity); \
	if (result != InnoGameSystemNS::g_GameSystemComponent->m_##className##sMap.end()) \
	{ \
		return result->second; \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem : can't find " + std::string(#className) + " by EntityID: " + std::to_string(parentEntity) + " !"); \
		return nullptr; \
	} \
}

getComponentImplDefi(TransformComponent)
getComponentImplDefi(VisibleComponent)
getComponentImplDefi(DirectionalLightComponent)
getComponentImplDefi(PointLightComponent)
getComponentImplDefi(SphereLightComponent)
getComponentImplDefi(CameraComponent)
getComponentImplDefi(InputComponent)
getComponentImplDefi(EnvironmentCaptureComponent)

INNO_SYSTEM_EXPORT void InnoGameSystem::registerButtonStatusCallback(InputComponent * inputComponent, ButtonData boundButton, std::function<void()>* function)
{
	auto l_kbuttonStatusCallbackVector = inputComponent->m_buttonStatusCallbackImpl.find(boundButton);
	if (l_kbuttonStatusCallbackVector != inputComponent->m_buttonStatusCallbackImpl.end())
	{
		l_kbuttonStatusCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_buttonStatusCallbackImpl.emplace(boundButton, std::vector<std::function<void()>*>{function});
	}
}

INNO_SYSTEM_EXPORT TransformComponent * InnoGameSystem::getRootTransformComponent()
{
	return InnoGameSystemNS::g_GameSystemComponent->m_rootTransformComponent;
}

INNO_SYSTEM_EXPORT void InnoGameSystem::registerMouseMovementCallback(InputComponent * inputComponent, int mouseCode, std::function<void(float)>* function)
{
	auto l_mouseMovementCallbackVector = inputComponent->m_mouseMovementCallbackImpl.find(mouseCode);
	if (l_mouseMovementCallbackVector != inputComponent->m_mouseMovementCallbackImpl.end())
	{
		l_mouseMovementCallbackVector->second.emplace_back(function);
	}
	else
	{
		inputComponent->m_mouseMovementCallbackImpl.emplace(mouseCode, std::vector<std::function<void(float)>*>{function});
	}
}

INNO_SYSTEM_EXPORT ObjectStatus InnoGameSystem::getStatus()
{
	return InnoGameSystemNS::m_objectStatus;
}
