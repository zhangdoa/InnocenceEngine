#include "FileSystem.h"

#include"../component/GameSystemComponent.h"
#include"../component/AssetSystemComponent.h"
#include "../component/RenderingSystemComponent.h"

#include "json/json.hpp"
using json = nlohmann::json;

namespace fs = std::filesystem;

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "stb/stb_image.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"

#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLFrameBufferComponent.h"
#include "../component/GLShaderProgramComponent.h"
#include "../component/GLRenderPassComponent.h"

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"
#include "../component/DXShaderProgramComponent.h"
#include "../component/DXRenderPassComponent.h"
#endif

#include "../component/PhysicsDataComponent.h"

#include "../../engine/system/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::string loadTextFile(const std::string & fileName);

	void to_json(json& j, const enitityNamePair& p);

	void to_json(json& j, const TransformComponent& p);
	void to_json(json& j, const TransformVector& p);
	void to_json(json& j, const VisibleComponent& p);
	void to_json(json& j, const vec4& p);
	void to_json(json& j, const DirectionalLightComponent& p);
	void to_json(json& j, const PointLightComponent& p);
	void to_json(json& j, const SphereLightComponent& p);
	void to_json(json& j, const EnvironmentCaptureComponent& p);

	void from_json(const json& j, TransformComponent& p);
	void from_json(const json& j, TransformVector& p);
	void from_json(const json& j, VisibleComponent& p);
	void from_json(const json& j, vec4& p);
	void from_json(const json& j, DirectionalLightComponent& p);
	void from_json(const json& j, PointLightComponent& p);
	void from_json(const json& j, SphereLightComponent& p);
	void from_json(const json& j, EnvironmentCaptureComponent& p);

	template<typename T>
	void loadComponentData(const json& j, const EntityID& entityID)
	{
		auto l_result = g_pCoreSystem->getGameSystem()->spawn<T>(entityID);

		from_json(j, *l_result);
	}

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
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GameSystem: Entity ID " + rhs->m_parentEntity + " is invalid when saving " + std::string(#className) + "."); \
				return false; \
			} \
	}
	template<>
	saveComponentDataDefi(TransformComponent);
	template<>
	saveComponentDataDefi(VisibleComponent);
	template<>
	saveComponentDataDefi(DirectionalLightComponent);
	template<>
	saveComponentDataDefi(PointLightComponent);
	template<>
	saveComponentDataDefi(SphereLightComponent);
	template<>
	saveComponentDataDefi(EnvironmentCaptureComponent);

	bool saveJsonDataToDisk(const std::string & fileName, const json & data);

	bool prepareForLoadingScene(const std::string& fileName);
	bool loadScene(const std::string& fileName);
	bool cleanScene();
	bool saveScene(const std::string& fileName);

	std::string convertModelFromDisk(const std::string & fileName, const std::string & exportPath);
	json processAssimpScene(const aiScene* aiScene, const std::string & exportPath);
	json processAssimpNode(const aiNode * node, const aiScene * scene, const std::string & exportPath);
	json processSingleAssimpMesh(const aiScene * scene, unsigned int meshIndex, const std::string & exportPath);
	EntityID processMeshVertices(const aiMesh * aiMesh, const std::string & exportPath);
	EntityID processMeshIndices(const aiMesh * aiMesh, const std::string & exportPath);
	json processSingleAssimpMaterial(const aiMaterial * aiMaterial, const std::string & exportPath);
	EntityID loadTextureFromDisk(const std::string& fileName, TextureUsageType TextureUsageType, const std::string & exportPath);

	bool serialize(std::ostream& os, void* ptr, size_t size)
	{
		os.write((char*)ptr, size);
		return true;
	}

	template<typename T>
	bool serializeVector(std::ostream& os, const std::vector<T>& vector)
	{
		serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		return true;
	}

	template<typename T>
	auto deserializeVector(const std::string& fileName) -> std::vector<T>
	{
		std::ifstream l_file(fileName, std::ios::binary);

		// get pointer to associated buffer object
		std::filebuf* pbuf = l_file.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
		pbuf->pubseekpos(0, l_file.in);

		std::vector<T> l_result(l_size / sizeof(T));
		pbuf->sgetn((char*)&l_result[0], l_size);

		auto x = l_result;

		l_file.close();
		return l_result;
	}

	std::vector<InnoFuture<void>> m_asyncTask;

	std::string m_nextLoadingScene;
	ThreadSafeQueue <std::pair<TransformComponent*, std::string>> m_orphanTransformComponents;
	ThreadSafeQueue <std::pair<CameraComponent*, std::string>> m_orphanCameraComponents;
	ThreadSafeQueue <std::pair<InputComponent*, std::string>> m_orphanInputComponents;
}

std::string InnoFileSystemNS::loadTextFile(const std::string & fileName)
{
	std::ifstream file;
	file.open((fileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

void InnoFileSystemNS::to_json(json& j, const enitityNamePair& p)
{
	j = json{
		{"EntityID", p.first},
	{"EntityName", p.second},
	};
}

void InnoFileSystemNS::to_json(json& j, const TransformVector& p)
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

void InnoFileSystemNS::to_json(json& j, const vec4& p)
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

void InnoFileSystemNS::to_json(json& j, const TransformComponent& p)
{
	json localTransformVector;

	to_json(localTransformVector, p.m_localTransformVector);

	auto parentTransformComponentEntityName = g_pCoreSystem->getGameSystem()->getEntityName(p.m_parentTransformComponent->m_parentEntity);

	j = json
	{
		{"ComponentType", InnoUtility::getComponentType<TransformComponent>()},
		{"ParentTransformComponentEntityName", parentTransformComponentEntityName},
		{"LocalTransformVector",
			localTransformVector
		},
	};
}

void InnoFileSystemNS::to_json(json& j, const VisibleComponent& p)
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

void InnoFileSystemNS::to_json(json& j, const DirectionalLightComponent& p)
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

void InnoFileSystemNS::to_json(json& j, const PointLightComponent& p)
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

void InnoFileSystemNS::to_json(json& j, const SphereLightComponent& p)
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

void InnoFileSystemNS::to_json(json& j, const EnvironmentCaptureComponent& p)
{
	j = json
	{ 
		{"ComponentType", InnoUtility::getComponentType<EnvironmentCaptureComponent>()},
		{"CubemapName", p.m_cubemapTextureFileName},
	};
}

void InnoFileSystemNS::from_json(const json & j, TransformComponent & p)
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
		m_orphanTransformComponents.push({ &p, l_parentTransformComponentEntityName });
	}
}

void InnoFileSystemNS::from_json(const json & j, TransformVector & p)
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

void InnoFileSystemNS::from_json(const json & j, VisibleComponent & p)
{
	p.m_visiblilityType = j["VisiblilityType"];
	p.m_meshShapeType = j["MeshShapeType"];
	p.m_meshDrawMethod = j["MeshPrimitiveTopology"];
	p.m_textureWrapMethod = j["TextureWrapMethod"];
	p.m_drawAABB = j["drawAABB"];
	p.m_modelFileName = j["ModelFileName"];
}

void InnoFileSystemNS::from_json(const json & j, vec4 & p)
{
	p.x = j["R"];
	p.y = j["G"];
	p.z = j["B"];
	p.w = j["A"];
}

void InnoFileSystemNS::from_json(const json & j, DirectionalLightComponent & p)
{
	p.m_luminousFlux = j["LuminousFlux"];
	p.m_drawAABB = j["drawAABB"];
	from_json(j["Color"], p.m_color);
}

void InnoFileSystemNS::from_json(const json & j, PointLightComponent & p)
{
	p.m_luminousFlux = j["LuminousFlux"];
	from_json(j["Color"], p.m_color);
}

void InnoFileSystemNS::from_json(const json & j, SphereLightComponent & p)
{
	p.m_luminousFlux = j["LuminousFlux"];
	p.m_sphereRadius = j["SphereRadius"];
	from_json(j["Color"], p.m_color);
}

void InnoFileSystemNS::from_json(const json & j, EnvironmentCaptureComponent & p)
{
	p.m_cubemapTextureFileName = j["CubemapName"];
}

bool InnoFileSystemNS::saveJsonDataToDisk(const std::string & fileName, const json & data)
{
	std::ofstream o;
	o.open(fileName, std::ios::out | std::ios::trunc);
	o << std::setw(4) << data << std::endl;
	o.close();

	return true;
}

bool InnoFileSystemNS::prepareForLoadingScene(const std::string& fileName)
{
	m_nextLoadingScene = fileName;
	GameSystemComponent::get().m_isLoadingScene = true;

	return true;
}

bool InnoFileSystemNS::loadScene(const std::string& fileName)
{
	std::ifstream i(fileName);

	if (!i.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open scene : " + fileName + " !");

		return false;
	}

	json j;
	i >> j;

	i.close();

	cleanScene();

	auto l_sceneName = j["SceneName"];

	for (auto& i : j["SceneEntities"])
	{
		if (i["EntityName"] != "RootTransform")
		{
			GameSystemComponent::get().m_enitityNameMap.erase(i["EntityID"]);

			auto l_EntityID = g_pCoreSystem->getGameSystem()->createEntity(i["EntityName"]);

			for (auto& k : i["ChildrenComponents"])
			{
				switch (componentType(k["ComponentType"]))
				{
				case componentType::TransformComponent: loadComponentData<TransformComponent>(k, l_EntityID);
					break;
				case componentType::VisibleComponent: loadComponentData<VisibleComponent>(k, l_EntityID);
					break;
				case componentType::DirectionalLightComponent: loadComponentData<DirectionalLightComponent>(k, l_EntityID);
					break;
				case componentType::PointLightComponent: loadComponentData<PointLightComponent>(k, l_EntityID);
					break;
				case componentType::SphereLightComponent: loadComponentData<SphereLightComponent>(k, l_EntityID);
					break;
				case componentType::CameraComponent:
					break;
				case componentType::InputComponent:
					break;
				case componentType::EnvironmentCaptureComponent: loadComponentData<EnvironmentCaptureComponent>(k, l_EntityID);
					break;
				case componentType::PhysicsDataComponent:
					break;
				case componentType::MeshDataComponent:
					break;
				case componentType::MaterialDataComponent:
					break;
				case componentType::TextureDataComponent:
					break;
				default:
					break;
				}
			}
		}
	}

	while (InnoFileSystemNS::m_orphanTransformComponents.size() > 0)
	{
		std::pair<TransformComponent*, std::string> l_orphan;
		if (InnoFileSystemNS::m_orphanTransformComponents.tryPop(l_orphan))
		{
			auto t = g_pCoreSystem->getGameSystem()->get<TransformComponent>(g_pCoreSystem->getGameSystem()->getEntityID(l_orphan.second));
			if (t)
			{
				l_orphan.first->m_parentTransformComponent = t;
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't find TransformComponent with entity name" + l_orphan.second + " !");
			}
		}
	}
	while (InnoFileSystemNS::m_orphanCameraComponents.size() > 0)
	{
		std::pair<CameraComponent*, std::string> l_orphan;
		if (InnoFileSystemNS::m_orphanCameraComponents.tryPop(l_orphan))
		{
			l_orphan.first->m_parentEntity = g_pCoreSystem->getGameSystem()->getEntityID(l_orphan.second);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: reattached CameraComponent to entity " + l_orphan.second + ".");
		}
	}
	while (InnoFileSystemNS::m_orphanInputComponents.size() > 0)
	{
		std::pair<InputComponent*, std::string> l_orphan;
		if (InnoFileSystemNS::m_orphanInputComponents.tryPop(l_orphan))
		{
			l_orphan.first->m_parentEntity = g_pCoreSystem->getGameSystem()->getEntityID(l_orphan.second);
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: reattached InputComponent to entity " + l_orphan.second + ".");
		}
	}

	g_pCoreSystem->getAssetSystem()->loadAssetsForComponents();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene " + fileName + " has been loaded.");

	return true;
}

bool InnoFileSystemNS::cleanScene()
{
	// cache components which can't be serilized currently
	for (auto i : GameSystemComponent::get().m_CameraComponents)
	{
		m_orphanCameraComponents.push(std::pair<CameraComponent*, std::string>(i, g_pCoreSystem->getGameSystem()->getEntityName(i->m_parentEntity)));
	}
	for (auto i : GameSystemComponent::get().m_InputComponents)
	{
		m_orphanInputComponents.push(std::pair<InputComponent*, std::string>(i, g_pCoreSystem->getGameSystem()->getEntityName(i->m_parentEntity)));
	}

	for (auto i : GameSystemComponent::get().m_TransformComponents)
	{
		g_pCoreSystem->getGameSystem()->destroy(i);
	}
	GameSystemComponent::get().m_TransformComponents.clear();
	GameSystemComponent::get().m_TransformComponentsMap.clear();

	for (auto i : GameSystemComponent::get().m_VisibleComponents)
	{
		g_pCoreSystem->getGameSystem()->destroy(i);
	}
	GameSystemComponent::get().m_VisibleComponents.clear();
	GameSystemComponent::get().m_VisibleComponentsMap.clear();

	for (auto i : GameSystemComponent::get().m_DirectionalLightComponents)
	{
		g_pCoreSystem->getGameSystem()->destroy(i);
	}
	GameSystemComponent::get().m_DirectionalLightComponents.clear();
	GameSystemComponent::get().m_DirectionalLightComponentsMap.clear();

	for (auto i : GameSystemComponent::get().m_PointLightComponents)
	{
		g_pCoreSystem->getGameSystem()->destroy(i);
	}
	GameSystemComponent::get().m_PointLightComponents.clear();
	GameSystemComponent::get().m_PointLightComponentsMap.clear();

	for (auto i : GameSystemComponent::get().m_SphereLightComponents)
	{
		g_pCoreSystem->getGameSystem()->destroy(i);
	}
	GameSystemComponent::get().m_SphereLightComponents.clear();
	GameSystemComponent::get().m_SphereLightComponentsMap.clear();

	for (auto i : GameSystemComponent::get().m_EnvironmentCaptureComponents)
	{
		g_pCoreSystem->getGameSystem()->destroy(i);
	}
	GameSystemComponent::get().m_EnvironmentCaptureComponents.clear();
	GameSystemComponent::get().m_EnvironmentCaptureComponentsMap.clear();

	return true;
}

bool InnoFileSystemNS::saveScene(const std::string& fileName)
{
	json topLevel;
	topLevel["SceneName"] = fileName;

	// save entities name and ID
	for (auto& i : GameSystemComponent::get().m_enitityNameMap)
	{
		json j;
		to_json(j, i);
		topLevel["SceneEntities"].emplace_back(j);
	}

	// save childern components
	for (auto i : GameSystemComponent::get().m_TransformComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : GameSystemComponent::get().m_VisibleComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : GameSystemComponent::get().m_DirectionalLightComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : GameSystemComponent::get().m_PointLightComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : GameSystemComponent::get().m_SphereLightComponents)
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : GameSystemComponent::get().m_EnvironmentCaptureComponents)
	{
		saveComponentData(topLevel, i);
	}

	saveJsonDataToDisk(fileName, topLevel);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene " + fileName + " has been saved.");

	return true;
}

std::string InnoFileSystemNS::convertModelFromDisk(const std::string & fileName, const std::string & exportPath)
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;

	if (fs::exists(fs::path(fileName)))
	{
		l_assScene = l_assImporter.ReadFile(fileName, aiProcess_Triangulate | aiProcess_FlipUVs);
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: " + fileName + " doesn't exist!");
		return std::string();
	}
	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return std::string();
	}

	auto l_exportFileName = fs::path(fileName).stem().generic_string();
	auto l_result = processAssimpScene(l_assScene, exportPath);
	saveJsonDataToDisk(exportPath + l_exportFileName + ".InnoAsset", l_result);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: " + fileName + " has been converted.");

	return l_exportFileName;
}

json InnoFileSystemNS::processAssimpScene(const aiScene* aiScene, const std::string & exportPath)
{
	json l_sceneData;

	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		l_sceneData["Nodes"].emplace_back(processAssimpNode(aiScene->mRootNode, aiScene, exportPath));
	}
	for (unsigned int i = 0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			l_sceneData["Nodes"].emplace_back(processAssimpNode(aiScene->mRootNode->mChildren[i], aiScene, exportPath));
		}
	}
	return l_sceneData;
}

json InnoFileSystemNS::processAssimpNode(const aiNode * node, const aiScene * scene, const std::string & exportPath)
{
	json l_nodeData;

	l_nodeData["NodeName"] = *node->mName.C_Str();

	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		l_nodeData["Meshes"].emplace_back(processSingleAssimpMesh(scene, node->mMeshes[i], exportPath));
	}

	return l_nodeData;
}

json InnoFileSystemNS::processSingleAssimpMesh(const aiScene * scene, unsigned int meshIndex, const std::string & exportPath)
{
	json l_meshData;

	auto l_aiMesh = scene->mMeshes[meshIndex];

	l_meshData["MeshName"] = *l_aiMesh->mName.C_Str();
	l_meshData["VeticesNumber"] = l_aiMesh->mNumVertices;

	auto l_verticesFileName = processMeshVertices(l_aiMesh, exportPath);
	l_meshData["MeshVerticesFile"] = l_verticesFileName.c_str();

	auto l_indicesFileName = processMeshIndices(l_aiMesh, exportPath);
	l_meshData["MeshIndicesFile"] = l_indicesFileName.c_str();

	// process material
	if (l_aiMesh->mMaterialIndex > 0)
	{
		l_meshData["Material"] = processSingleAssimpMaterial(scene->mMaterials[l_aiMesh->mMaterialIndex], exportPath);
	}

	return l_meshData;
}

EntityID InnoFileSystemNS::processMeshVertices(const aiMesh * aiMesh, const std::string & exportPath)
{
	auto l_verticesNumber = aiMesh->mNumVertices;

	std::vector<Vertex> l_vertices;

	l_vertices.reserve(l_verticesNumber);

	for (unsigned int i = 0; i < l_verticesNumber; i++)
	{
		Vertex l_Vertex;

		// positions
		if (&aiMesh->mVertices[i] != nullptr)
		{
			l_Vertex.m_pos.x = aiMesh->mVertices[i].x;
			l_Vertex.m_pos.y = aiMesh->mVertices[i].y;
			l_Vertex.m_pos.z = aiMesh->mVertices[i].z;
		}
		else
		{
			l_Vertex.m_pos.x = 0.0f;
			l_Vertex.m_pos.y = 0.0f;
			l_Vertex.m_pos.z = 0.0f;
		}

		// texture coordinates
		if (aiMesh->mTextureCoords[0])
		{
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			l_Vertex.m_texCoord.x = aiMesh->mTextureCoords[0][i].x;
			l_Vertex.m_texCoord.y = aiMesh->mTextureCoords[0][i].y;
		}
		else
		{
			l_Vertex.m_texCoord.x = 0.0f;
			l_Vertex.m_texCoord.y = 0.0f;
		}

		// normals
		if (aiMesh->mNormals)
		{
			l_Vertex.m_normal.x = aiMesh->mNormals[i].x;
			l_Vertex.m_normal.y = aiMesh->mNormals[i].y;
			l_Vertex.m_normal.z = aiMesh->mNormals[i].z;
		}
		else
		{
			l_Vertex.m_normal.x = 0.0f;
			l_Vertex.m_normal.y = 0.0f;
			l_Vertex.m_normal.z = 0.0f;
		}
		l_vertices.emplace_back(l_Vertex);
	}

	l_vertices.shrink_to_fit();

	auto l_exportFileName = InnoMath::createEntityID();
	std::ofstream l_file(exportPath + l_exportFileName + ".InnoMeshVertices", std::ios::binary);
	serializeVector(l_file, l_vertices);
	l_file.close();

	return l_exportFileName;
}

EntityID InnoFileSystemNS::processMeshIndices(const aiMesh* aiMesh, const std::string & exportPath)
{
	std::vector<Index> l_indices;
	size_t l_indiceSize = 0;

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
	{
		aiFace l_face = aiMesh->mFaces[i];
		l_indiceSize += l_face.mNumIndices;
	}
	l_indices.reserve(l_indiceSize);

	for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
	{
		aiFace l_face = aiMesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < l_face.mNumIndices; j++)
		{
			l_indices.emplace_back(l_face.mIndices[j]);
		}
	}

	auto l_exportFileName = InnoMath::createEntityID();

	std::ofstream l_file(exportPath + l_exportFileName + ".InnoMeshIndices", std::ios::binary);
	serializeVector(l_file, l_indices);
	l_file.close();

	return l_exportFileName;
}

/*
aiTextureType::aiTextureType_NORMALS TextureUsageType::NORMAL map_Kn normal map texture
aiTextureType::aiTextureType_DIFFUSE TextureUsageType::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR TextureUsageType::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_AMBIENT TextureUsageType::ROUGHNESS map_Ka roughness texture
aiTextureType::aiTextureType_EMISSIVE TextureUsageType::AMBIENT_OCCLUSION map_emissive AO texture
*/

json InnoFileSystemNS::processSingleAssimpMaterial(const aiMaterial * aiMaterial, const std::string & exportPath)
{
	json l_materialData;

	for (unsigned int i = 0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			aiString l_AssString;
			aiMaterial->GetTexture(aiTextureType(i), 0, &l_AssString);
			std::string l_localPath = std::string(l_AssString.C_Str());

			if (aiTextureType(i) == aiTextureType::aiTextureType_NONE)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: ASSIMP: " + l_localPath + " is unknown texture type!");
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
			{
				l_materialData["Normal Texture"] = loadTextureFromDisk(l_localPath, TextureUsageType::NORMAL, exportPath).c_str();
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				l_materialData["Albedo Texture"] = loadTextureFromDisk(l_localPath, TextureUsageType::ALBEDO, exportPath).c_str();
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				l_materialData["Metallic Texture"] = loadTextureFromDisk(l_localPath, TextureUsageType::METALLIC, exportPath).c_str();
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				l_materialData["Roughness Texture"] = loadTextureFromDisk(l_localPath, TextureUsageType::ROUGHNESS, exportPath).c_str();
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				l_materialData["AO Texture"] = loadTextureFromDisk(l_localPath, TextureUsageType::AMBIENT_OCCLUSION, exportPath).c_str();
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: ASSIMP: " + l_localPath + " is unsupported texture type!");
			}
		}
	}

	auto l_result = aiColor3D();

	if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Albedo"] =
		{
			{"R", l_result.r},
			{"G", l_result.g},
			{"B", l_result.b},
		};
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Metallic"] = l_result.r;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Roughness"] = l_result.r;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["AO"] = l_result.r;
	}

	return l_materialData;
}

EntityID InnoFileSystemNS::loadTextureFromDisk(const std::string& fileName, TextureUsageType TextureUsageType, const std::string & exportPath)
{
	int width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	auto l_filePath = fileName;

	TextureDataDesc l_textureDataDesc;

	void* l_rawData;
	auto l_isHDR = stbi_is_hdr(l_filePath.c_str());

	if (l_isHDR)
	{
		l_rawData = stbi_loadf(l_filePath.c_str(), &width, &height, &nrChannels, 0);
	}
	else
	{
		l_rawData = stbi_load(l_filePath.c_str(), &width, &height, &nrChannels, 0);
	}
	if (l_rawData)
	{
		auto l_exportFileName = InnoMath::createEntityID();

		l_textureDataDesc.textureUsageType = TextureUsageType;
		l_textureDataDesc.textureColorComponentsFormat = l_isHDR ? TextureColorComponentsFormat((unsigned int)TextureColorComponentsFormat::R16F + (nrChannels - 1)) : TextureColorComponentsFormat((nrChannels - 1));
		l_textureDataDesc.texturePixelDataFormat = TexturePixelDataFormat(nrChannels - 1);
		l_textureDataDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
		l_textureDataDesc.textureMinFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
		l_textureDataDesc.textureMagFilterMethod = TextureFilterMethod::LINEAR;
		l_textureDataDesc.texturePixelDataType = l_isHDR ? TexturePixelDataType::FLOAT : TexturePixelDataType::UNSIGNED_BYTE;
		l_textureDataDesc.textureWidth = width;
		l_textureDataDesc.textureHeight = height;

		std::ofstream l_file(exportPath + l_exportFileName + ".InnoTexture", std::ios::binary);

		l_file.write((char*)&l_textureDataDesc, sizeof(l_textureDataDesc));
		l_file.write((char*)l_rawData, l_textureDataDesc.textureWidth * l_textureDataDesc.textureHeight);

		l_file.close();

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "AssimpWrapper: STB_Image: " + fileName + " has been converted.");

		return l_exportFileName;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "AssimpWrapper: STB_Image: Failed to load texture: " + fileName);

		return EntityID();
	}
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::setup()
{
	g_pCoreSystem->getAssetSystem()->loadDefaultAssets();

	InnoFileSystemNS::m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::update()
{
	if (GameSystemComponent::get().m_isLoadingScene)
	{
		InnoFileSystemNS::loadScene(InnoFileSystemNS::m_nextLoadingScene);
		GameSystemComponent::get().m_isLoadingScene = false;
	}
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::terminate()
{
	InnoFileSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;

	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoFileSystem::getStatus()
{
	return InnoFileSystemNS::m_objectStatus;
}

std::string InnoFileSystem::loadTextFile(const std::string & fileName)
{
	return InnoFileSystemNS::loadTextFile(fileName);
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::loadDefaultScene()
{
	InnoFileSystemNS::loadScene("..//res//scenes//default.InnoScene");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::loadScene(const std::string & fileName)
{
	return InnoFileSystemNS::prepareForLoadingScene(fileName);
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::saveScene(const std::string & fileName)
{
	return InnoFileSystemNS::saveScene(fileName);
}

void InnoFileSystem::saveComponentToDiskImpl(componentType type, size_t classSize, void* ptr, const std::string& fileName)
{
	if (!ptr) return;
	char* l_ptr_raw = reinterpret_cast<char*>(ptr);
	unsigned int l_engineVer = 7;
	auto l_time = g_pCoreSystem->getTimeSystem()->getCurrentTime();

	std::ofstream l_file;
	l_file.open(fileName + ".InnoAsset", std::ios::out | std::ios::trunc | std::ios::binary);
	l_file.write((char*)&type, sizeof(type));
	l_file.write((char*)&l_engineVer, sizeof(l_engineVer));
	l_file.write((char*)&l_time, sizeof(l_time));
	l_file.write(l_ptr_raw, classSize);

	l_file.close();
}

void* InnoFileSystem::loadComponentFromDiskImpl(const std::string& fileName)
{
	std::ifstream l_file;
	l_file.open(fileName + ".InnoAsset", std::ios::binary);

	if (!l_file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: Can't open file " + fileName + " for deserialization!");
		return nullptr;
	}

	// get pointer to associated buffer object
	std::filebuf* pbuf = l_file.rdbuf();
	// get file size using buffer's members
	std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
	pbuf->pubseekpos(0, l_file.in);

	// allocate memory to contain file data
	// @TODO: new???
	char* buffer = new char[l_size];

	// get file data
	pbuf->sgetn(buffer, l_size);

	char* l_ptr;
	auto l_classType = *(componentType*)buffer;
	size_t classSize;

	switch (l_classType)
	{
	case componentType::TransformComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<TransformComponent>());
		classSize = sizeof(TransformComponent);
		break;
	case componentType::VisibleComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<VisibleComponent>());
		classSize = sizeof(VisibleComponent);
		break;
	case componentType::DirectionalLightComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<DirectionalLightComponent>());
		classSize = sizeof(DirectionalLightComponent);
		break;
	case componentType::PointLightComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<PointLightComponent>());
		classSize = sizeof(PointLightComponent);
		break;
	case componentType::SphereLightComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<SphereLightComponent>());
		classSize = sizeof(SphereLightComponent);
		break;
	case componentType::CameraComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<CameraComponent>());
		classSize = sizeof(CameraComponent);
		break;
	case componentType::InputComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<InputComponent>());
		classSize = sizeof(InputComponent);
		break;
	case componentType::EnvironmentCaptureComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<EnvironmentCaptureComponent>());
		classSize = sizeof(EnvironmentCaptureComponent);
		break;
	case componentType::PhysicsDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<PhysicsDataComponent>());
		classSize = sizeof(PhysicsDataComponent);
		break;
	case componentType::MeshDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<MeshDataComponent>());
		classSize = sizeof(MeshDataComponent);
		break;
	case componentType::MaterialDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<MaterialDataComponent>());
		classSize = sizeof(MaterialDataComponent);
		break;
	case componentType::TextureDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>());
		classSize = sizeof(TextureDataComponent);
		break;
	default:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: Unsupported deserialization data type " + std::to_string((int)l_classType) + " !");
		return nullptr;
		break;
	}

	std::memcpy(l_ptr, buffer + 16, classSize);

	l_file.close();

	delete[] buffer;

	return l_ptr;
}