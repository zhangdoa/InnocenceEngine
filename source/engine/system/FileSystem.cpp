#include "FileSystem.h"

#include "json/json.hpp"
using json = nlohmann::json;

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"

#include "../../engine/system/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "stb/stb_image.h"

#include "../common/stl17.h"
#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif



INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	namespace AssimpWrapper
	{
		bool convertModel(const std::string & fileName, const std::string & exportPath);
		json processAssimpScene(const aiScene* aiScene);
		json processAssimpNode(const aiNode * node, const aiScene * scene);
		json processAssimpMesh(const aiScene * scene, unsigned int meshIndex);
		std::pair<EntityID, size_t> processMeshData(const aiMesh * aiMesh);
		json processAssimpMaterial(const aiMaterial * aiMaterial);
		json processTextureData(const std::string & fileName, TextureSamplerType samplerType, TextureUsageType usageType);
	};

	namespace ModelLoader
	{
		ModelMap loadModelFromDisk(const std::string & fileName);
		ModelMap processSceneJsonData(const json & j);
		ModelMap processNodeJsonData(const json & j);
		ModelPair processMeshJsonData(const json& j);
		MaterialDataComponent* processMaterialJsonData(const json& j);
		TextureDataComponent* loadTexture(const std::string& fileName);
		TextureDataComponent* loadTextureFromDisk(const std::string & fileName);
	}

	std::string loadTextFile(const std::string & fileName);
	std::vector<char> loadBinaryFile(const std::string & fileName);

	bool isFileExist(const std::string & fileName);

	bool convertModel(const std::string & fileName, const std::string & exportPath);

	void to_json(json& j, const EntityNamePair& p);

	void to_json(json& j, const TransformComponent& p);
	void to_json(json& j, const TransformVector& p);
	void to_json(json& j, const VisibleComponent& p);
	void to_json(json& j, const vec4& p);
	void to_json(json& j, const DirectionalLightComponent& p);
	void to_json(json& j, const PointLightComponent& p);
	void to_json(json& j, const SphereLightComponent& p);
	void to_json(json& j, const CameraComponent& p);
	void to_json(json& j, const EnvironmentCaptureComponent& p);

	void from_json(const json& j, TransformComponent& p);
	void from_json(const json& j, TransformVector& p);
	void from_json(const json& j, VisibleComponent& p);
	void from_json(const json& j, vec4& p);
	void from_json(const json& j, DirectionalLightComponent& p);
	void from_json(const json& j, PointLightComponent& p);
	void from_json(const json& j, SphereLightComponent& p);
	void from_json(const json& j, CameraComponent& p);
	void from_json(const json& j, EnvironmentCaptureComponent& p);

	template<typename T>
	inline bool loadComponentData(const json& j, const EntityID& entityID)
	{
		auto l_result = g_pCoreSystem->getGameSystem()->spawn<T>(entityID);

		from_json(j, *l_result);

		return true;
	}

	template<typename T>
	inline bool saveComponentData(json& topLevel, T* rhs)
	{
		json j;
		to_json(j, *rhs);

		auto result = std::find_if(
			topLevel["SceneEntities"].begin(),
			topLevel["SceneEntities"].end(),
			[&](auto& val) -> bool {
			return val["EntityID"] == rhs->m_parentEntity;
		});

		if (result != topLevel["SceneEntities"].end())
		{
			result.value()["ChildrenComponents"].emplace_back(j);
			return true;
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: saveComponentData<T>: Entity ID " + rhs->m_parentEntity + " is invalid.");
			return false;
		}
	}

	bool loadJsonDataFromDisk(const std::string & fileName, json & data);
	bool saveJsonDataToDisk(const std::string & fileName, const json & data);

	bool prepareForLoadingScene(const std::string& fileName);
	bool loadScene(const std::string& fileName);
	bool loadComponents(const json& j);
	bool assignComponentRuntimeData();
	bool loadAssets();
	bool saveScene(const std::string& fileName);

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

	bool deserialize(std::istream& is, void* ptr)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, is.end, is.in);
		pbuf->pubseekpos(0, is.in);
		pbuf->sgetn((char*)ptr, l_size);
		return true;
	}

	template<typename T>
	bool deserializeVector(std::istream& is, std::streamoff startPos, std::size_t size, std::vector<T>& vector)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		pbuf->pubseekpos(startPos, is.in);

		auto rhs = std::vector<T>(size / sizeof(T));

		pbuf->sgetn((char*)&rhs[0], size);
		vector = std::move(rhs);
		return true;
	}

	template<typename T>
	bool deserializeVector(std::istream& is, std::vector<T>& vector)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, is.end, is.in);
		pbuf->pubseekpos(0, is.in);
		auto rhs = std::vector<T>(l_size / sizeof(T));

		pbuf->sgetn((char*)&rhs[0], l_size);
		vector = std::move(rhs);
		return true;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::vector<InnoFuture<void>> m_asyncTask;
	std::vector<std::function<void()>*> m_sceneLoadingStartCallbacks;
	std::vector<std::function<void()>*> m_sceneLoadingFinishCallbacks;

	std::string m_workingDir;
	std::atomic<bool> m_isLoadingScene = false;
	std::atomic<bool> m_prepareForLoadingScene = false;

	std::string m_nextLoadingScene;
	std::string m_currentScene;

	ThreadSafeQueue<std::pair<TransformComponent*, std::string>> m_orphanTransformComponents;

	std::unordered_map<std::string, ModelMap> m_loadedModelMap;
	std::unordered_map<std::string, ModelPair> m_loadedModelPair;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTexture;
}

std::string InnoFileSystemNS::loadTextFile(const std::string & fileName)
{
	std::ifstream file;

	file.open((m_workingDir + fileName).c_str());

	if (!file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open text file : " + fileName + "!");
		return std::string();
	}

	std::stringstream ss;
	std::string output;

	ss << file.rdbuf();
	output = ss.str();
	file.close();

	return output;
}

std::vector<char> InnoFileSystemNS::loadBinaryFile(const std::string & fileName)
{
	std::ifstream file;
	file.open((m_workingDir + fileName).c_str(), std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open binary file : " + fileName + "!");
		return std::vector<char>();
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

bool InnoFileSystemNS::isFileExist(const std::string & fileName)
{
	if (fs::exists(fs::path(m_workingDir + fileName)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool InnoFileSystemNS::convertModel(const std::string & fileName, const std::string & exportPath)
{
	auto l_extension = fs::path(fileName).extension().generic_string();
	if (l_extension == ".obj")
	{
		auto tempTask = g_pCoreSystem->getTaskSystem()->submit([=]()
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: converting " + fileName + " ...");
			AssimpWrapper::convertModel(fileName, exportPath);
		});
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: " + fileName + " is not supported!");

		return false;
	}
	return true;
}

void InnoFileSystemNS::to_json(json& j, const EntityNamePair& p)
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
		{"MeshUsageType", p.m_meshUsageType},
		{"MeshPrimitiveTopology", p.m_meshPrimitiveTopology},
		{"TextureWrapMethod", p.m_textureWrapMethod},
		{"ModelFileName", p.m_modelFileName},
		{"SimulatePhysics", p.m_simulatePhysics},
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

void InnoFileSystemNS::to_json(json& j, const CameraComponent& p)
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
	p.m_meshUsageType = j["MeshUsageType"];
	p.m_meshPrimitiveTopology = j["MeshPrimitiveTopology"];
	p.m_textureWrapMethod = j["TextureWrapMethod"];
	p.m_modelFileName = j["ModelFileName"];
	p.m_simulatePhysics = j["SimulatePhysics"];
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

void InnoFileSystemNS::from_json(const json& j, CameraComponent& p)
{
	p.m_FOVX = j["FOVX"];
	p.m_widthScale = j["widthScale"];
	p.m_heightScale = j["heightScale"];
	p.m_zNear = j["zNear"];
	p.m_zFar = j["zFar"];
}

void InnoFileSystemNS::from_json(const json & j, EnvironmentCaptureComponent & p)
{
	p.m_cubemapTextureFileName = j["CubemapName"];
}

bool InnoFileSystemNS::loadJsonDataFromDisk(const std::string & fileName, json & data)
{
	std::ifstream i(m_workingDir + fileName);

	if (!i.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open JSON file : " + fileName + "!");
		return false;
	}

	i >> data;
	i.close();

	return true;
}

bool InnoFileSystemNS::saveJsonDataToDisk(const std::string & fileName, const json & data)
{
	std::ofstream o;
	o.open(m_workingDir + fileName, std::ios::out | std::ios::trunc);
	o << std::setw(4) << data << std::endl;
	o.close();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: JSON file : " + fileName + " has been saved.");

	return true;
}

bool InnoFileSystemNS::prepareForLoadingScene(const std::string& fileName)
{
	if (!InnoFileSystemNS::m_isLoadingScene)
	{
		if (m_currentScene == fileName)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: scene " + fileName + " has already loaded now.");
			return true;
		}
		m_nextLoadingScene = fileName;
		m_prepareForLoadingScene = true;
	}

	return true;
}

bool InnoFileSystemNS::loadScene(const std::string& fileName)
{
	for (auto i : m_sceneLoadingStartCallbacks)
	{
		(*i)();
	}

	json j;
	if (!loadJsonDataFromDisk(fileName, j))
	{
		return false;
	}

	loadComponents(j);

	assignComponentRuntimeData();

	for (auto i : m_sceneLoadingFinishCallbacks)
	{
		(*i)();
	}

	InnoFileSystemNS::m_isLoadingScene = false;

	m_currentScene = fileName;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene " + fileName + " has been loaded.");

	loadAssets();

	return true;
}

bool InnoFileSystemNS::loadComponents(const json& j)
{
	auto l_sceneName = j["SceneName"];

	for (auto i : j["SceneEntities"])
	{
		if (i["EntityName"] != "RootTransform")
		{
			g_pCoreSystem->getGameSystem()->removeEntity(i["EntityName"]);

			auto l_EntityID = g_pCoreSystem->getGameSystem()->createEntity(i["EntityName"]);

			for (auto k : i["ChildrenComponents"])
			{
				switch (ComponentType(k["ComponentType"]))
				{
				case ComponentType::TransformComponent: loadComponentData<TransformComponent>(k, l_EntityID);
					break;
				case ComponentType::VisibleComponent: loadComponentData<VisibleComponent>(k, l_EntityID);
					break;
				case ComponentType::DirectionalLightComponent: loadComponentData<DirectionalLightComponent>(k, l_EntityID);
					break;
				case ComponentType::PointLightComponent: loadComponentData<PointLightComponent>(k, l_EntityID);
					break;
				case ComponentType::SphereLightComponent: loadComponentData<SphereLightComponent>(k, l_EntityID);
					break;
				case ComponentType::CameraComponent: loadComponentData<CameraComponent>(k, l_EntityID);
					break;
				case ComponentType::InputComponent:
					break;
				case ComponentType::EnvironmentCaptureComponent: loadComponentData<EnvironmentCaptureComponent>(k, l_EntityID);
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
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: components loading finished.");

	return true;
}

bool InnoFileSystemNS::assignComponentRuntimeData()
{
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
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't find TransformComponent with entity name" + l_orphan.second + "!");
			}
		}
	}

	return true;
}

bool InnoFileSystemNS::loadAssets()
{
	g_pCoreSystem->getAssetSystem()->loadAssetsForComponents();

	return true;
}

bool InnoFileSystemNS::saveScene(const std::string& fileName)
{
	json topLevel;
	topLevel["SceneName"] = fileName;

	// save entities name and ID
	for (auto& i : g_pCoreSystem->getGameSystem()->getEntityNameMap())
	{
		json j;
		to_json(j, i);
		topLevel["SceneEntities"].emplace_back(j);
	}

	// save childern components
	for (auto i : g_pCoreSystem->getGameSystem()->get<TransformComponent>())
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<VisibleComponent>())
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<DirectionalLightComponent>())
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<PointLightComponent>())
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<SphereLightComponent>())
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<CameraComponent>())
	{
		saveComponentData(topLevel, i);
	}
	for (auto i : g_pCoreSystem->getGameSystem()->get<EnvironmentCaptureComponent>())
	{
		saveComponentData(topLevel, i);
	}

	saveJsonDataToDisk(fileName, topLevel);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene " + fileName + " has been saved.");

	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::setup()
{
#if defined INNO_PLATFORM_WIN
	InnoFileSystemNS::m_workingDir = fs::current_path().parent_path().generic_string();
#else
	InnoFileSystemNS::m_workingDir = fs::current_path().generic_string();
#endif
	InnoFileSystemNS::m_workingDir = InnoFileSystemNS::m_workingDir + "//";
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: current working directory is " + InnoFileSystemNS::m_workingDir);
	InnoFileSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::update()
{
	if (InnoFileSystemNS::m_prepareForLoadingScene)
	{
		InnoFileSystemNS::m_prepareForLoadingScene = false;
		InnoFileSystemNS::m_isLoadingScene = true;
		g_pCoreSystem->getTaskSystem()->waitAllTasksToFinish();
		InnoFileSystemNS::loadScene(InnoFileSystemNS::m_nextLoadingScene);
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

INNO_SYSTEM_EXPORT std::string InnoFileSystem::getWorkingDirectory()
{
	return InnoFileSystemNS::m_workingDir;
}

std::string InnoFileSystem::loadTextFile(const std::string & fileName)
{
	return InnoFileSystemNS::loadTextFile(fileName);
}

INNO_SYSTEM_EXPORT std::vector<char> InnoFileSystem::loadBinaryFile(const std::string & fileName)
{
	return InnoFileSystemNS::loadBinaryFile(fileName);
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::loadScene(const std::string & fileName)
{
	return InnoFileSystemNS::prepareForLoadingScene(fileName);
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::saveScene(const std::string & fileName)
{
	return InnoFileSystemNS::saveScene(fileName);
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::isLoadingScene()
{
	return InnoFileSystemNS::m_isLoadingScene;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::addSceneLoadingStartCallback(std::function<void()>* functor)
{
	InnoFileSystemNS::m_sceneLoadingStartCallbacks.emplace_back(functor);
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::addSceneLoadingFinishCallback(std::function<void()>* functor)
{
	InnoFileSystemNS::m_sceneLoadingFinishCallbacks.emplace_back(functor);
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::convertModel(const std::string & fileName, const std::string & exportPath)
{
	return InnoFileSystemNS::convertModel(fileName, exportPath);
}

INNO_SYSTEM_EXPORT ModelMap InnoFileSystem::loadModel(const std::string & fileName)
{
	auto l_extension = fs::path(fileName).extension().generic_string();
	if (l_extension == ".InnoModel")
	{
		return InnoFileSystemNS::ModelLoader::loadModelFromDisk(fileName);
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: AssimpWrapper: " + fileName + " is not supported!");
		return ModelMap();
	}
}

INNO_SYSTEM_EXPORT TextureDataComponent* InnoFileSystem::loadTexture(const std::string & fileName)
{
	return InnoFileSystemNS::ModelLoader::loadTexture(fileName);
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::addCPPClassFiles(const CPPClassDesc & desc)
{
	// Build header file
	auto l_headerFileName = desc.filePath + desc.className + ".h";
	std::ofstream l_headerFile(InnoFileSystemNS::m_workingDir + l_headerFileName, std::ios::out | std::ios::trunc);

	if (!l_headerFile.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: std::ofstream: can't open file " + l_headerFileName + "!");
		return false;
	}

	// Common headers include
	l_headerFile << "#pragma once" << std::endl;
	l_headerFile << "#include \"common/InnoType.h\"" << std::endl;
	l_headerFile << "#include \"common/InnoClassTemplate.h\"" << std::endl;
	l_headerFile << std::endl;

	// Abstraction type
	if (desc.isInterface)
	{
		l_headerFile << "INNO_INTERFACE ";
	}
	else
	{
		l_headerFile << "INNO_CONCRETE ";
	}

	l_headerFile << desc.className;

	// Inheriance type
	if (!desc.parentClass.empty())
	{
		l_headerFile << " : INNO_IMPLEMENT " << desc.parentClass;
	}

	l_headerFile << std::endl;

	// Class decl body
	l_headerFile << "{" << std::endl;
	l_headerFile << "public:" << std::endl;

	// Ctor type
	if (desc.isInterface)
	{
		if (desc.isNonMoveable && desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_COPYABLE_AND_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonMoveable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_COPYABLE(" << desc.className << ");" << std::endl;
		}
		else
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_DEFALUT(" << desc.className << ");" << std::endl;
		}
	}
	else
	{
		if (desc.isNonMoveable && desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_COPYABLE_AND_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonMoveable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_COPYABLE(" << desc.className << ");" << std::endl;
		}
		else
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_DEFALUT(" << desc.className << ");" << std::endl;
		}
	}

	l_headerFile << std::endl;
	l_headerFile << "  bool setup();" << std::endl;
	l_headerFile << "  bool initialize();" << std::endl;
	l_headerFile << "  bool update();" << std::endl;
	l_headerFile << "  bool terminate();" << std::endl;
	l_headerFile << "  ObjectStatus getStatus();" << std::endl;

	l_headerFile << std::endl;
	l_headerFile << "private:" << std::endl;
	l_headerFile << "  ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;" << std::endl;
	l_headerFile << "};" << std::endl;

	l_headerFile.close();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: " + l_headerFileName + " has been generated.");
	return true;
}

bool InnoFileSystemNS::AssimpWrapper::convertModel(const std::string & fileName, const std::string & exportPath)
{
	auto l_exportFileName = fs::path(fileName).stem().generic_string();
	auto l_exportFileRelativePath = exportPath + l_exportFileName + ".InnoModel";

	if (isFileExist(l_exportFileRelativePath))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: AssimpWrapper: " + fileName + " has already been converted!");
		return true;
	}

	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;

	if (isFileExist(fileName))
	{
		l_assScene = l_assImporter.ReadFile(m_workingDir + fileName, aiProcess_Triangulate | aiProcess_FlipUVs);
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssimpWrapper: " + fileName + " doesn't exist!");
		return false;
	}
	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssimpWrapper: " + std::string{ l_assImporter.GetErrorString() });
		return false;
	}

	auto l_result = processAssimpScene(l_assScene);
	saveJsonDataToDisk(l_exportFileRelativePath, l_result);

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: AssimpWrapper: " + fileName + " has been converted.");

	return true;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpScene(const aiScene* aiScene)
{
	auto l_timeData = g_pCoreSystem->getTimeSystem()->getCurrentTime();
	auto l_timeDataStr =
		"["
		+ std::to_string(l_timeData.year)
		+ "-" + std::to_string(l_timeData.month)
		+ "-" + std::to_string(l_timeData.day)
		+ "-" + std::to_string(l_timeData.hour)
		+ "-" + std::to_string(l_timeData.minute)
		+ "-" + std::to_string(l_timeData.second)
		+ "-" + std::to_string(l_timeData.millisecond)
		+ "]";

	json l_sceneData;

	l_sceneData["Timestamp"] = l_timeDataStr;

	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		l_sceneData["Nodes"].emplace_back(processAssimpNode(aiScene->mRootNode, aiScene));
	}
	for (unsigned int i = 0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			l_sceneData["Nodes"].emplace_back(processAssimpNode(aiScene->mRootNode->mChildren[i], aiScene));
		}
	}
	return l_sceneData;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpNode(const aiNode * node, const aiScene * scene)
{
	json l_nodeData;

	l_nodeData["NodeName"] = *node->mName.C_Str();

	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		l_nodeData["Meshes"].emplace_back(processAssimpMesh(scene, node->mMeshes[i]));
	}

	return l_nodeData;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpMesh(const aiScene * scene, unsigned int meshIndex)
{
	json l_meshData;

	auto l_aiMesh = scene->mMeshes[meshIndex];

	l_meshData["MeshName"] = *l_aiMesh->mName.C_Str();
	l_meshData["VerticesNumber"] = l_aiMesh->mNumVertices;
	auto l_meshFileName = processMeshData(l_aiMesh);
	l_meshData["MeshFile"] = l_meshFileName.first.c_str();
	l_meshData["IndicesNumber"] = l_meshFileName.second;

	// process material
	if (l_aiMesh->mMaterialIndex > 0)
	{
		l_meshData["Material"] = processAssimpMaterial(scene->mMaterials[l_aiMesh->mMaterialIndex]);
	}

	return l_meshData;
}

std::pair<EntityID, size_t> InnoFileSystemNS::AssimpWrapper::processMeshData(const aiMesh * aiMesh)
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

	std::string l_exportFileName;

	if (aiMesh->mName.length)
	{
		l_exportFileName = aiMesh->mName.C_Str();
	}
	else
	{
		l_exportFileName = InnoMath::createEntityID();
	}

	auto l_exportFileRelativePath = "//res//convertedAssets//" + l_exportFileName + ".InnoRaw";

	std::ofstream l_file(m_workingDir + l_exportFileRelativePath, std::ios::binary);

	serializeVector(l_file, l_vertices);
	serializeVector(l_file, l_indices);

	l_file.close();

	return std::pair<EntityID, size_t>(l_exportFileRelativePath, l_indiceSize);
}

/*
aiTextureType::aiTextureType_NORMALS TextureUsageType::NORMAL map_Kn normal map texture
aiTextureType::aiTextureType_DIFFUSE TextureUsageType::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR TextureUsageType::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_AMBIENT TextureUsageType::ROUGHNESS map_Ka roughness texture
aiTextureType::aiTextureType_EMISSIVE TextureUsageType::AMBIENT_OCCLUSION map_emissive AO texture
aiTextureType::AI_MATKEY_COLOR_DIFFUSE Kd Albedo RGB
aiTextureType::AI_MATKEY_COLOR_TRANSPARENT Ks Alpha A
aiTextureType::AI_MATKEY_COLOR_SPECULAR Ka Metallic
aiTextureType::AI_MATKEY_COLOR_AMBIENT Ke Roughness
aiTextureType::AI_MATKEY_COLOR_EMISSIVE AO
aiTextureType::AI_MATKEY_COLOR_REFLECTIVE Thickness
*/

json InnoFileSystemNS::AssimpWrapper::processAssimpMaterial(const aiMaterial * aiMaterial)
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
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: AssimpWrapper: " + l_localPath + " is unknown texture type!");
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION));
			}
			else
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: AssimpWrapper: " + l_localPath + " is unsupported texture type!");
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
	else
	{
		l_materialData["Albedo"] =
		{
			{"R", 1.0f},
			{"G", 1.0f},
			{"B", 1.0f},
		};
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Albedo"]["A"] = l_result.r;
	}
	else
	{
		l_materialData["Albedo"]["A"] = 1.0f;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Metallic"] = l_result.r;
	}
	else
	{
		l_materialData["Metallic"] = 0.5f;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Roughness"] = l_result.r;
	}
	else
	{
		l_materialData["Roughness"] = 0.5f;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["AO"] = l_result.r;
	}
	else
	{
		l_materialData["AO"] = 1.0f;
	}
	if (aiMaterial->Get(AI_MATKEY_COLOR_REFLECTIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		l_materialData["Thickness"] = l_result.r;
	}
	else
	{
		l_materialData["Thickness"] = 1.0f;
	}
	return l_materialData;
}

json InnoFileSystemNS::AssimpWrapper::processTextureData(const std::string & fileName, TextureSamplerType samplerType, TextureUsageType usageType)
{
	json j;

	j["SamplerType"] = samplerType;
	j["UsageType"] = usageType;
	j["File"] = fileName;

	return j;
}

ModelMap InnoFileSystemNS::ModelLoader::loadModelFromDisk(const std::string & fileName)
{
	ModelMap l_result;

	// check if this file has already been loaded once
	auto l_loadedModelMap = m_loadedModelMap.find(fileName);
	if (l_loadedModelMap != m_loadedModelMap.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: ModelLoader: " + fileName + " has been already loaded.");
		// Just copy new materials
		for (auto& i : l_loadedModelMap->second)
		{
			auto l_material = g_pCoreSystem->getAssetSystem()->addMaterialDataComponent();
			*l_material = *i.second;
			l_result.emplace(i.first, l_material);
		}
		return l_result;
	}

	json j;

	if (loadJsonDataFromDisk(fileName, j))
	{
		l_result = std::move(processSceneJsonData(j));
		m_loadedModelMap.emplace(fileName, l_result);
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: ModelLoader: can't load " + fileName + "!");
	}

	return l_result;
}

ModelMap InnoFileSystemNS::ModelLoader::processSceneJsonData(const json & j)
{
	ModelMap l_SceneResult;

	for (auto i : j["Nodes"])
	{
		auto l_nodeResult = std::move(processNodeJsonData(i));
		for (auto i : l_nodeResult)
		{
			l_SceneResult.emplace(i);
		}
	}

	return l_SceneResult;
}

ModelMap InnoFileSystemNS::ModelLoader::processNodeJsonData(const json & j)
{
	ModelMap l_nodeResult;

	for (auto i : j["Meshes"])
	{
		l_nodeResult.emplace(processMeshJsonData(i));
	}

	return l_nodeResult;
}

ModelPair InnoFileSystemNS::ModelLoader::processMeshJsonData(const json & j)
{
	ModelPair l_result;

	auto l_meshFileName = j["MeshFile"].get<std::string>();

	auto l_loadedModelPair = m_loadedModelPair.find(l_meshFileName);
	if (l_loadedModelPair != m_loadedModelPair.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: ModelLoader: " + l_meshFileName + " has been already loaded.");
		l_result = l_loadedModelPair->second;
	}
	else
	{
		std::ifstream l_meshFile(m_workingDir + l_meshFileName, std::ios::binary);

		if (!l_meshFile.is_open())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: std::ifstream: can't open file " + l_meshFileName + "!");
			return ModelPair();
		}

		auto l_MeshDC = g_pCoreSystem->getAssetSystem()->addMeshDataComponent();

		size_t l_verticesNumber = j["VerticesNumber"];
		size_t l_indicesNumber = j["IndicesNumber"];

		deserializeVector(l_meshFile, 0, l_verticesNumber * sizeof(Vertex), l_MeshDC->m_vertices);

		deserializeVector(l_meshFile, l_verticesNumber * sizeof(Vertex), l_indicesNumber * sizeof(Index), l_MeshDC->m_indices);

		l_meshFile.close();

		l_MeshDC->m_indicesSize = l_MeshDC->m_indices.size();
		l_MeshDC->m_meshShapeType = MeshShapeType::CUSTOM;
		l_MeshDC->m_objectStatus = ObjectStatus::STANDBY;

		g_pCoreSystem->getPhysicsSystem()->generatePhysicsDataComponent(l_MeshDC);

		l_result.first = l_MeshDC;
		l_result.second = processMaterialJsonData(j["Material"]);

		m_loadedModelPair.emplace(l_meshFileName, l_result);

		g_pCoreSystem->getVisionSystem()->getRenderingBackend()->registerUninitializedMeshDataComponent(l_MeshDC);
	}

	return l_result;
}

MaterialDataComponent * InnoFileSystemNS::ModelLoader::processMaterialJsonData(const json & j)
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->addMaterialDataComponent();

	if (j.find("Textures") != j.end())
	{
		for (auto i : j["Textures"])
		{
			auto l_TDC = loadTexture(i["File"]);
			if (l_TDC)
			{
				l_TDC->m_textureDataDesc.samplerType = TextureSamplerType(i["SamplerType"]);
				l_TDC->m_textureDataDesc.usageType = TextureUsageType(i["UsageType"]);
			}
			else
			{
				l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType(i["UsageType"]));
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

TextureDataComponent* InnoFileSystemNS::ModelLoader::loadTexture(const std::string& fileName)
{
	TextureDataComponent* l_TDC;

	auto l_loadedTDC = m_loadedTexture.find(fileName);
	if (l_loadedTDC != m_loadedTexture.end())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: ModelLoader: " + fileName + " has been already loaded.");
		l_TDC = l_loadedTDC->second;
	}
	else
	{
		l_TDC = loadTextureFromDisk(fileName);
	}

	return l_TDC;
}

TextureDataComponent* InnoFileSystemNS::ModelLoader::loadTextureFromDisk(const std::string & fileName)
{
	int width, height, nrChannels;

	// load image, flip texture
	stbi_set_flip_vertically_on_load(true);

	void* l_rawData;
	auto l_fullPath = m_workingDir + fileName;
	auto l_isHDR = stbi_is_hdr(l_fullPath.c_str());

	if (l_isHDR)
	{
		l_rawData = stbi_loadf(l_fullPath.c_str(), &width, &height, &nrChannels, 0);
	}
	else
	{
		l_rawData = stbi_load(l_fullPath.c_str(), &width, &height, &nrChannels, 0);
	}
	if (l_rawData)
	{
		auto l_TDC = g_pCoreSystem->getAssetSystem()->addTextureDataComponent();

		l_TDC->m_textureDataDesc.pixelDataFormat = TexturePixelDataFormat(nrChannels - 1);
		l_TDC->m_textureDataDesc.wrapMethod = TextureWrapMethod::REPEAT;
		l_TDC->m_textureDataDesc.minFilterMethod = TextureFilterMethod::LINEAR_MIPMAP_LINEAR;
		l_TDC->m_textureDataDesc.magFilterMethod = TextureFilterMethod::LINEAR;
		l_TDC->m_textureDataDesc.pixelDataType = l_isHDR ? TexturePixelDataType::FLOAT16 : TexturePixelDataType::UBYTE;
		l_TDC->m_textureDataDesc.width = width;
		l_TDC->m_textureDataDesc.height = height;
		l_TDC->m_textureData.emplace_back(l_rawData);

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: ModelLoader: STB_Image: " + l_fullPath + " has been loaded.");

		m_loadedTexture.emplace(fileName, l_TDC);

		g_pCoreSystem->getVisionSystem()->getRenderingBackend()->registerUninitializedTextureDataComponent(l_TDC);

		return l_TDC;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: ModelLoader: STB_Image: Failed to load texture: " + l_fullPath);

		return nullptr;
	}
}
