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

bool AssimpImporter::Import(const char* fileName, const char* exportPath)
{
	auto l_exportFileName = g_Engine->Get<IOService>()->getFileName(fileName);

	// read file via ASSIMP
	Assimp::Importer l_importer;
	const aiScene* l_scene;

	// Check if the file exists
	if (g_Engine->Get<IOService>()->isFileExist(fileName))
	{
		Log(Verbose, "Converting ", fileName, "...");
#if defined INNO_DEBUG
		std::string l_logFilePath = g_Engine->Get<IOService>()->getWorkingDirectory() + "..//Res//Logs//AssimpLog_" + l_exportFileName + ".txt";
		Assimp::DefaultLogger::create(l_logFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif
		l_scene = l_importer.ReadFile(g_Engine->Get<IOService>()->getWorkingDirectory() + fileName,
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
	}
	else
	{
		Log(Error, "", fileName, " doesn't exist!");
		return false;
	}

	if (l_scene)
	{
		if (l_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_scene->mRootNode)
		{
			Log(Error, "", l_importer.GetErrorString());
			return false;
		}

		nlohmann::json j;
		ProcessAssimpScene(j, l_scene, l_exportFileName.c_str());

		Log(Success, "", fileName, " has been converted.");
	}
	else
	{
		Log(Error, "Can't load file ", fileName, "!");
		return false;
	}

	return true;
}

void AssimpImporter::ProcessAssimpScene(nlohmann::json& j, const aiScene* scene, const char* exportName)
{
	Log(Verbose, "Creating model component for: ", exportName);

	// Create temporary entity using EntityManager
	auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, "TempModelEntity");

	auto l_modelComponent = g_Engine->Get<ComponentManager>()->Spawn<ModelComponent>(l_tempEntity, true, ObjectLifespan::Scene);
	l_modelComponent->m_UUID = Randomizer::GenerateUUID();
	l_modelComponent->m_ObjectStatus = ObjectStatus::Created;

	// Process meshes and create DrawCallComponents
	ProcessAssimpNode(scene->mRootNode, scene, exportName, l_modelComponent);

	// Save ModelComponent without subfolders and using GetTypeName()
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_modelPath = l_workingDir + "Data/Components/" + std::string(exportName) + "." + ModelComponent::GetTypeName() + ".inno";
	AssetService::Save(l_modelPath.c_str(), *l_modelComponent);

	// Clean up temporary entity
	g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);

	Log(Success, "Model conversion complete: ", exportName);
}

void AssimpImporter::ProcessAssimpNode(const aiNode* node, const aiScene* scene, const char* exportName, ModelComponent* modelComponent)
{
	// Process each mesh located at the current node
	if (node->mNumMeshes)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			auto l_meshIndex = node->mMeshes[i];
			auto l_mesh = scene->mMeshes[l_meshIndex];

			Log(Verbose, "Processing mesh: ", l_mesh->mName.C_Str());

			// Create temporary entity using EntityManager
			auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, "TempDrawCallEntity");

			// Create DrawCallComponent for this mesh
			auto l_drawCallComponent = g_Engine->Get<ComponentManager>()->Spawn<DrawCallComponent>(l_tempEntity, true, ObjectLifespan::Scene);
			l_drawCallComponent->m_UUID = Randomizer::GenerateUUID();
			l_drawCallComponent->m_ObjectStatus = ObjectStatus::Created;

			// Create and save MeshComponent
			auto l_meshComponent = AssimpMeshProcessor::CreateMeshComponent(scene, exportName, l_meshIndex);
			if (l_meshComponent)
			{
				l_drawCallComponent->m_MeshComponent = l_meshComponent->m_UUID;
			}

			// Create and save MaterialComponent if material exists
			if (l_mesh->mMaterialIndex < scene->mNumMaterials)
			{
				auto l_materialComponent = AssimpMaterialProcessor::CreateMaterialComponent(scene->mMaterials[l_mesh->mMaterialIndex], exportName);
				if (l_materialComponent)
				{
					l_drawCallComponent->m_MaterialComponent = l_materialComponent->m_UUID;
				}
			}

			// Save DrawCallComponent without subfolders and using GetTypeName()
			auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
			auto l_drawCallName = std::string(exportName) + "_drawcall_" + std::to_string(l_meshIndex);
			auto l_drawCallPath = l_workingDir + "Data/Components/" + l_drawCallName + "." + DrawCallComponent::GetTypeName() + ".inno";
			AssetService::Save(l_drawCallPath.c_str(), *l_drawCallComponent);

			// Clean up temporary entity
			g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);

			// Add to ModelComponent's DrawCallComponents list (store UUID)
			modelComponent->m_DrawCallComponents.emplace_back(l_drawCallComponent->m_UUID);
		}
	}

	// Process children nodes recursively
	if (node->mNumChildren)
	{
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			ProcessAssimpNode(node->mChildren[i], scene, exportName, modelComponent);
		}
	}
}
