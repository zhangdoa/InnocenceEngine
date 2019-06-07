#include "JSONParser.h"

#include "../ICoreSystem.h"
extern ICoreSystem* g_pCoreSystem;

#include "FileSystemHelper.h"
#include "AssetLoader.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS::JSONParser
{
	template<typename T>
	inline bool loadComponentData(const json& j, const InnoEntity* entity)
	{
		auto l_result = g_pCoreSystem->getGameSystem()->spawn<T>(entity, ObjectSource::Asset, ObjectUsage::Gameplay);
		from_json(j, *l_result);

		return true;
	}

	template<typename T>
	inline bool saveComponentData(json& topLevel, T* rhs)
	{
		json j;
		to_json(j, *rhs);

		auto l_result = std::find_if(
			topLevel["SceneEntities"].begin(),
			topLevel["SceneEntities"].end(),
			[&](auto val) -> bool {
				return val["EntityID"] == rhs->m_parentEntity->m_entityID.c_str();
			});

			if (l_result != topLevel["SceneEntities"].end())
			{
				l_result.value()["ChildrenComponents"].emplace_back(j);
				return true;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: saveComponentData<T>: Entity ID " + std::string(rhs->m_parentEntity->m_entityID.c_str()) + " is invalid.");
				return false;
			}
		}

		ModelMap processNodeJsonData(const json & j);
		ModelPair processMeshJsonData(const json& j);
		SkeletonDataComponent* processSkeletonJsonData(const std::string& skeletonFileName);
		MaterialDataComponent* processMaterialJsonData(const std::string& materialFileName);

		bool assignComponentRuntimeData();

		std::unordered_map<std::string, ModelPair> m_loadedModelPair;
		ThreadSafeQueue<std::pair<TransformComponent*, EntityName>> m_orphanTransformComponents;
	}

	bool InnoFileSystemNS::JSONParser::loadJsonDataFromDisk(const std::string & fileName, json & data)
	{
		std::ifstream i(getWorkingDirectory() + fileName);

		if (!i.is_open())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open JSON file : " + fileName + "!");
			return false;
		}

		i >> data;
		i.close();

		return true;
	}

	bool InnoFileSystemNS::JSONParser::saveJsonDataToDisk(const std::string & fileName, const json & data)
	{
		std::ofstream o;
		o.open(getWorkingDirectory() + fileName, std::ios::out | std::ios::trunc);
		o << std::setw(4) << data << std::endl;
		o.close();

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: JSON file : " + fileName + " has been saved.");

		return true;
	}

	void InnoFileSystemNS::JSONParser::to_json(json& j, const InnoEntity& p)
	{
		j = json{
			{"EntityID", p.m_entityID.c_str()},
			{"EntityName", p.m_entityName.c_str()},
		};
	}

	void InnoFileSystemNS::JSONParser::to_json(json& j, const TransformVector& p)
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
			},
			{
				"Scale",
				{
					{
						"X", p.m_scale.x
					},
					{
						"Y", p.m_scale.y
					},
					{
						"Z", p.m_scale.z
					},
				}
			}
		};
	}

	void InnoFileSystemNS::JSONParser::to_json(json& j, const vec4& p)
	{
		j = json
		{
			{
				"X", p.x
			},
			{
				"Y", p.y
			},
			{
				"Z", p.z
			},
			{
				"W", p.w
			}
		};
	}

	void InnoFileSystemNS::JSONParser::to_json(json& j, const TransformComponent& p)
	{
		json localTransformVector;

		to_json(localTransformVector, p.m_localTransformVector);

		auto parentTransformComponentEntityName = p.m_parentTransformComponent->m_parentEntity->m_entityName;

		j = json
		{
			{"ComponentType", InnoUtility::getComponentType<TransformComponent>()},
			{"ParentTransformComponentEntityName", parentTransformComponentEntityName.c_str()},
			{"LocalTransformVector",
			localTransformVector
		},
	};
}

void InnoFileSystemNS::JSONParser::to_json(json& j, const VisibleComponent& p)
{
	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<VisibleComponent>()},
		{"VisiblilityType", p.m_visiblilityType},
		{"MeshShapeType", p.m_meshShapeType},
		{"MeshUsageType", p.m_meshUsageType},
		{"MeshPrimitiveTopology", p.m_meshPrimitiveTopology},
		{"TextureWrapMethod", p.m_textureWrapMethod},
		{"ModelFileName", p.m_modelFileName},
		{"SimulatePhysics", p.m_simulatePhysics},
	};
}

void InnoFileSystemNS::JSONParser::to_json(json& j, const DirectionalLightComponent& p)
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

void InnoFileSystemNS::JSONParser::to_json(json& j, const PointLightComponent& p)
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

void InnoFileSystemNS::JSONParser::to_json(json& j, const SphereLightComponent& p)
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

void InnoFileSystemNS::JSONParser::to_json(json& j, const CameraComponent& p)
{
	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<CameraComponent>()},
		{"FOVX", p.m_FOVX},
		{"widthScale", p.m_widthScale},
		{"heightScale", p.m_heightScale},
		{"zNear", p.m_zNear},
		{"zFar", p.m_zFar},
	};
}

void InnoFileSystemNS::JSONParser::to_json(json& j, const EnvironmentCaptureComponent& p)
{
	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<EnvironmentCaptureComponent>()},
		{"CubemapName", p.m_cubemapTextureFileName},
	};
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, TransformComponent & p)
{
	from_json(j["LocalTransformVector"], p.m_localTransformVector);
	auto l_parentTransformComponentEntityName = j["ParentTransformComponentEntityName"];
	if (l_parentTransformComponentEntityName == "RootTransform")
	{
		p.m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
	}
	else
	{
		// JSON is an order-irrelevant format, so the parent transform component would always be instanciated in random point, then it's necessary to assign it later
		std::string l_parentName = l_parentTransformComponentEntityName;
		l_parentName += "/";
		m_orphanTransformComponents.push({ &p, EntityName(l_parentName.c_str()) });
	}
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, TransformVector & p)
{
	p.m_pos.x = j["Position"]["X"];
	p.m_pos.y = j["Position"]["Y"];
	p.m_pos.z = j["Position"]["Z"];
	p.m_pos.w = 1.0f;

	p.m_rot.x = j["Rotation"]["X"];
	p.m_rot.y = j["Rotation"]["Y"];
	p.m_rot.z = j["Rotation"]["Z"];
	p.m_rot.w = j["Rotation"]["W"];

	p.m_scale.x = j["Scale"]["X"];
	p.m_scale.y = j["Scale"]["Y"];
	p.m_scale.z = j["Scale"]["Z"];
	p.m_scale.w = 1.0f;
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, VisibleComponent & p)
{
	p.m_visiblilityType = j["VisiblilityType"];
	p.m_meshShapeType = j["MeshShapeType"];
	p.m_meshUsageType = j["MeshUsageType"];
	p.m_meshPrimitiveTopology = j["MeshPrimitiveTopology"];
	p.m_textureWrapMethod = j["TextureWrapMethod"];
	p.m_modelFileName = j["ModelFileName"];
	p.m_simulatePhysics = j["SimulatePhysics"];
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, vec4 & p)
{
	p.x = j["X"];
	p.y = j["Y"];
	p.z = j["Z"];
	p.w = j["W"];
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, DirectionalLightComponent & p)
{
	p.m_luminousFlux = j["LuminousFlux"];
	p.m_drawAABB = j["drawAABB"];
	from_json(j["Color"], p.m_color);
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, PointLightComponent & p)
{
	p.m_luminousFlux = j["LuminousFlux"];
	from_json(j["Color"], p.m_color);
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, SphereLightComponent & p)
{
	p.m_luminousFlux = j["LuminousFlux"];
	p.m_sphereRadius = j["SphereRadius"];
	from_json(j["Color"], p.m_color);
}

void InnoFileSystemNS::JSONParser::from_json(const json& j, CameraComponent& p)
{
	p.m_FOVX = j["FOVX"];
	p.m_widthScale = j["widthScale"];
	p.m_heightScale = j["heightScale"];
	p.m_zNear = j["zNear"];
	p.m_zFar = j["zFar"];
}

void InnoFileSystemNS::JSONParser::from_json(const json & j, EnvironmentCaptureComponent & p)
{
	p.m_cubemapTextureFileName = j["CubemapName"];
}

ModelMap InnoFileSystemNS::JSONParser::loadModelFromDisk(const std::string & fileName)
{
	json j;

	loadJsonDataFromDisk(fileName, j);

	return  std::move(processNodeJsonData(j));
}

ModelMap InnoFileSystemNS::JSONParser::processNodeJsonData(const json & j)
{
	ModelMap l_nodeResult;

	if (j.find("Meshes") != j.end())
	{
		for (auto i : j["Meshes"])
		{
			l_nodeResult.emplace(processMeshJsonData(i));
		}
	}

	// children nodes
	if (j.find("Nodes") != j.end())
	{
		for (auto i : j["Nodes"])
		{
			auto l_childrenNodeResult = std::move(processNodeJsonData(i));
			for (auto i : l_childrenNodeResult)
			{
				l_nodeResult.emplace(i);
			}
		}
	}

	return l_nodeResult;
}

ModelPair InnoFileSystemNS::JSONParser::processMeshJsonData(const json & j)
{
	ModelPair l_result;

	// Load mesh data
	auto l_meshFileName = j["MeshFile"].get<std::string>();

	auto l_loadedModelPair = m_loadedModelPair.find(l_meshFileName);
	if (l_loadedModelPair != m_loadedModelPair.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: JSONParser: " + l_meshFileName + " has been already loaded.");
		l_result = l_loadedModelPair->second;
	}
	else
	{
		std::ifstream l_meshFile(getWorkingDirectory() + l_meshFileName, std::ios::binary);

		if (!l_meshFile.is_open())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: std::ifstream: can't open file " + l_meshFileName + "!");
			return ModelPair();
		}

		auto l_MeshDC = g_pCoreSystem->getRenderingFrontend()->addMeshDataComponent();

		size_t l_verticesNumber = j["VerticesNumber"];
		size_t l_indicesNumber = j["IndicesNumber"];

		deserializeVector(l_meshFile, 0, l_verticesNumber * sizeof(Vertex), l_MeshDC->m_vertices);

		deserializeVector(l_meshFile, l_verticesNumber * sizeof(Vertex), l_indicesNumber * sizeof(Index), l_MeshDC->m_indices);

		l_meshFile.close();

		l_MeshDC->m_indicesSize = l_MeshDC->m_indices.size();
		l_MeshDC->m_meshShapeType = MeshShapeType::CUSTOM;
		l_MeshDC->m_objectStatus = ObjectStatus::Created;

		g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_MeshDC);

		l_result.first = l_MeshDC;

		// Load skeleton data
		if (j.find("SkeletonFile") != j.end())
		{
			l_result.first->m_SDC = processSkeletonJsonData(j["SkeletonFile"]);
		}

		// Load material data
		if (j.find("MaterialFile") != j.end())
		{
			l_result.second = processMaterialJsonData(j["MaterialFile"]);
		}
		else
		{
			l_result.second = g_pCoreSystem->getRenderingFrontend()->addMaterialDataComponent();
		}

		m_loadedModelPair.emplace(l_meshFileName, l_result);

		g_pCoreSystem->getRenderingBackend()->registerUninitializedMeshDataComponent(l_MeshDC);
	}

	return l_result;
}

SkeletonDataComponent * InnoFileSystemNS::JSONParser::processSkeletonJsonData(const std::string& skeletonFileName)
{
	json j;

	loadJsonDataFromDisk(skeletonFileName, j);

	auto l_SDC = g_pCoreSystem->getRenderingFrontend()->addSkeletonDataComponent();
	auto l_size = j["Bones"].size();
	l_SDC->m_Bones.reserve(l_size);

	for (auto i : j["Bones"])
	{
		Bone l_bone;
		l_bone.m_ID = i["BoneID"];
		l_bone.m_Pos.x = i["OffsetPosition"]["X"];
		l_bone.m_Pos.y = i["OffsetPosition"]["Y"];
		l_bone.m_Pos.z = i["OffsetPosition"]["Z"];
		l_bone.m_Pos.w = i["OffsetPosition"]["W"];
		l_bone.m_Rot.x = i["OffsetRotation"]["X"];
		l_bone.m_Rot.y = i["OffsetRotation"]["Y"];
		l_bone.m_Rot.z = i["OffsetRotation"]["Z"];
		l_bone.m_Rot.w = i["OffsetRotation"]["W"];

		l_SDC->m_Bones.emplace_back(l_bone);
	}
	return l_SDC;
}

MaterialDataComponent * InnoFileSystemNS::JSONParser::processMaterialJsonData(const std::string& materialFileName)
{
	json j;

	loadJsonDataFromDisk(materialFileName, j);

	auto l_MDC = g_pCoreSystem->getRenderingFrontend()->addMaterialDataComponent();

	if (j.find("Textures") != j.end())
	{
		for (auto i : j["Textures"])
		{
			auto l_TDC = AssetLoader::loadTexture(i["File"]);
			if (l_TDC)
			{
				l_TDC->m_textureDataDesc.samplerType = TextureSamplerType(i["SamplerType"]);
				l_TDC->m_textureDataDesc.usageType = TextureUsageType(i["UsageType"]);
			}
			else
			{
				l_TDC = g_pCoreSystem->getRenderingFrontend()->getTextureDataComponent(TextureUsageType(i["UsageType"]));
			}
			switch (l_TDC->m_textureDataDesc.usageType)
			{
				case TextureUsageType::NORMAL: l_MDC->m_texturePack.m_normalTDC.second = l_TDC; break;
				case TextureUsageType::ALBEDO: l_MDC->m_texturePack.m_albedoTDC.second = l_TDC; break;
				case TextureUsageType::METALLIC: l_MDC->m_texturePack.m_metallicTDC.second = l_TDC; break;
				case TextureUsageType::ROUGHNESS: l_MDC->m_texturePack.m_roughnessTDC.second = l_TDC; break;
				case TextureUsageType::AMBIENT_OCCLUSION: l_MDC->m_texturePack.m_aoTDC.second = l_TDC; break;
				default:
				break;
			}
		}
	}

	l_MDC->m_meshCustomMaterial.albedo_r = j["Albedo"]["R"];
	l_MDC->m_meshCustomMaterial.albedo_g = j["Albedo"]["G"];
	l_MDC->m_meshCustomMaterial.albedo_b = j["Albedo"]["B"];
	l_MDC->m_meshCustomMaterial.alpha = j["Albedo"]["A"];
	l_MDC->m_meshCustomMaterial.metallic = j["Metallic"];
	l_MDC->m_meshCustomMaterial.roughness = j["Roughness"];
	l_MDC->m_meshCustomMaterial.ao = j["AO"];
	l_MDC->m_meshCustomMaterial.thickness = j["Thickness"];

	return l_MDC;
}

bool InnoFileSystemNS::JSONParser::saveScene(const std::string& fileName)
{
	json topLevel;
	topLevel["SceneName"] = fileName;

	// save entities name and ID
	for (auto i : g_pCoreSystem->getGameSystem()->getEntities())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			json j;
			to_json(j, *i);
			topLevel["SceneEntities"].emplace_back(j);
		}
	}

	// save children components
	for (auto i : g_pCoreSystem->getGameSystem()->get<TransformComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<CameraComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>())
	{
		if (i->m_objectSource == ObjectSource::Asset)
		{
			saveComponentData(topLevel, i);
		}
	}

	saveJsonDataToDisk(fileName, topLevel);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene " + fileName + " has been saved.");

	return true;
}

bool InnoFileSystemNS::JSONParser::loadScene(const std::string & fileName)
{
	json j;
	if (!loadJsonDataFromDisk(fileName, j))
	{
		return false;
	}

	auto l_sceneName = j["SceneName"];

	for (auto i : j["SceneEntities"])
	{
		std::string l_entityName = i["EntityName"];
		l_entityName += "/";

		auto l_entity = g_pCoreSystem->getGameSystem()->createEntity(EntityName(l_entityName.c_str()), ObjectSource::Asset, ObjectUsage::Gameplay);

		for (auto k : i["ChildrenComponents"])
		{
			switch (ComponentType(k["ComponentType"]))
			{
				case ComponentType::TransformComponent: loadComponentData<TransformComponent>(k, l_entity);
				break;
				case ComponentType::VisibleComponent: loadComponentData<VisibleComponent>(k, l_entity);
				break;
				case ComponentType::DirectionalLightComponent: loadComponentData<DirectionalLightComponent>(k, l_entity);
				break;
				case ComponentType::PointLightComponent: loadComponentData<PointLightComponent>(k, l_entity);
				break;
				case ComponentType::SphereLightComponent: loadComponentData<SphereLightComponent>(k, l_entity);
				break;
				case ComponentType::CameraComponent: loadComponentData<CameraComponent>(k, l_entity);
				break;
				case ComponentType::InputComponent:
				break;
				case ComponentType::EnvironmentCaptureComponent: loadComponentData<EnvironmentCaptureComponent>(k, l_entity);
				break;
				case ComponentType::PhysicsDataComponent:
				break;
				case ComponentType::MeshDataComponent:
				break;
				case ComponentType::MaterialDataComponent:
				break;
				case ComponentType::TextureDataComponent:
				break;
				default:
				break;
			}
		}
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene loading finished.");

	assignComponentRuntimeData();

	return true;
}

bool InnoFileSystemNS::JSONParser::assignComponentRuntimeData()
{
	while (m_orphanTransformComponents.size() > 0)
	{
		std::pair<TransformComponent*, EntityName> l_orphan;
		if (m_orphanTransformComponents.tryPop(l_orphan))
		{
			auto l_entity = g_pCoreSystem->getGameSystem()->getEntity(l_orphan.second);

			auto l_parentTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_entity);

			if (l_parentTransformComponent)
			{
				l_orphan.first->m_parentTransformComponent = l_parentTransformComponent;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't find TransformComponent with entity name" + std::string(l_orphan.second.c_str()) + "!");
			}
		}
	}

	return true;
}
