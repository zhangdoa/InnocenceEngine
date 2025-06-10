#include "AssimpImporter.h"
#include "AssimpMeshProcessor.h"
#include "AssimpMaterialProcessor.h"
#include "AssimpTextureProcessor.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/DefaultLogger.hpp"

#include "../../Common/LogService.h"
#include "../../Common/Timer.h"
#include "../../Common/IOService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Services/AssetService.h"
#include "../../Services/ComponentManager.h"
#include "../../Services/EntityManager.h"
#include "../../Engine.h"

using namespace Inno;

bool AssimpImporter::Import(const char* fileName)
{
	auto l_exportFileName = g_Engine->Get<IOService>()->getFileName(fileName);
	if (!g_Engine->Get<IOService>()->isFileExist(fileName))
	{
		Log(Error, "", fileName, " doesn't exist!");
		return false;
	}

	Log(Verbose, "Converting ", fileName, "...");
#if defined INNO_DEBUG
	std::string l_logFilePath = "AssimpLog_" + l_exportFileName + ".txt";
	Assimp::DefaultLogger::create(l_logFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif
	
	Assimp::Importer l_importer;
	const aiScene* l_scene = l_importer.ReadFile(fileName,
		aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_CalcTangentSpace
		| aiProcess_FlipUVs
		| aiProcess_JoinIdenticalVertices
		| aiProcess_SplitLargeMeshes
		//| aiProcess_FindInstances // Do not merge instances so the culling result could be more optimized
		| aiProcess_OptimizeMeshes
		| aiProcess_OptimizeGraph
	);

	if (!l_scene)
	{
		Log(Error, "Can't load file ", fileName, "!");
		return false;
	}

	if (l_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_scene->mRootNode)
	{
		Log(Error, "", l_importer.GetErrorString());
		return false;
	}

	nlohmann::json j;
	ProcessAssimpScene(j, l_scene, l_exportFileName.c_str());

	Log(Success, fileName, " has been imported.");
	return true;
}

void AssimpImporter::ProcessAssimpScene(nlohmann::json& j, const aiScene* scene, const char* exportName)
{
	Log(Verbose, "Creating model component for: ", exportName);

	auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, exportName);

	auto l_modelComponent = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(l_tempEntity, true, ObjectLifespan::Frame);
	l_modelComponent->m_UUID = Randomizer::GenerateUUID();
	l_modelComponent->m_ObjectStatus = ObjectStatus::Created;

	ProcessAssimpNode(scene->mRootNode, scene, exportName, l_modelComponent);

	AssetService::Save(*l_modelComponent);

	g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);

	Log(Success, "Model conversion complete: ", exportName);
}

void AssimpImporter::ProcessAssimpNode(const aiNode* node, const aiScene* scene, const char* baseName, ModelComponent* modelComponent)
{
	if (node->mNumMeshes)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			auto l_meshIndex = node->mMeshes[i];
			auto l_mesh = scene->mMeshes[l_meshIndex];

			Log(Verbose, "Processing mesh: ", l_mesh->mName.C_Str());

			auto l_name = std::string(baseName) + "." + std::to_string(l_meshIndex) + "/";
			auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, l_name.c_str());

			auto l_drawCallComponent = g_Engine->Get<ComponentManager>()->Spawn<DrawCallComponent>(l_tempEntity, true, ObjectLifespan::Frame);

			auto l_meshComponent = AssimpMeshProcessor::CreateMeshComponent(scene, baseName, l_meshIndex);
			if (l_meshComponent)
				l_drawCallComponent->m_MeshComponent = l_meshComponent->m_UUID;
			
			if (l_mesh->mMaterialIndex < scene->mNumMaterials)
			{
				auto l_materialComponent = AssimpMaterialProcessor::CreateMaterialComponent(scene->mMaterials[l_mesh->mMaterialIndex], baseName);
				if (l_materialComponent)
					l_drawCallComponent->m_MaterialComponent = l_materialComponent->m_UUID;
			}

			AssetService::Save(*l_drawCallComponent);
			
			g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);
			
			modelComponent->m_DrawCallComponents.emplace_back(l_drawCallComponent->m_UUID);
		}
	}

	if (node->mNumChildren)
	{
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessAssimpNode(node->mChildren[i], scene, baseName, modelComponent);
		}
	}
}
