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
#include "../../Engine.h"

using namespace Inno;

bool AssimpImporter::Import(const char* FileName)
{
	auto l_ExportFileName = g_Engine->Get<IOService>()->getFileName(FileName);
	if (!g_Engine->Get<IOService>()->isFileExist(FileName))
	{
		Log(Error, "", FileName, " doesn't exist!");
		return false;
	}

	Log(Verbose, "Converting ", FileName, "...");
#if defined INNO_DEBUG
	std::string l_LogFilePath = "AssimpLog_" + l_ExportFileName + ".txt";
	Assimp::DefaultLogger::create(l_LogFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif

	Assimp::Importer l_Importer;
	const aiScene* l_Scene = l_Importer.ReadFile(FileName,
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

	if (!l_Scene)
	{
		Log(Error, "Can't load file ", FileName, "!");
		return false;
	}

	if (l_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_Scene->mRootNode)
	{
		Log(Error, "", l_Importer.GetErrorString());
		return false;
	}

	nlohmann::json l_Json;
	ProcessAssimpScene(l_Json, l_Scene, l_ExportFileName.c_str());

	Log(Success, FileName, " has been imported.");
	return true;
}

void AssimpImporter::ProcessAssimpScene(nlohmann::json& J, const aiScene* Scene, const char* ExportName)
{
	Log(Verbose, "Processing scene: ", ExportName);

	ProcessAssimpNode(Scene->mRootNode, Scene, ExportName);

	Log(Success, "Model conversion complete: ", ExportName);
}

// AssimpWrapper is an offline asset converter (Baker tool).
// Components are populated from Assimp data and saved to disk by the processors.
// EntityRegistry entity creation happens at runtime load time in AssetService (Task 10).
void AssimpImporter::ProcessAssimpNode(const aiNode* Node, const aiScene* Scene, const char* BaseName)
{
	if (Node->mNumMeshes)
	{
		for (uint32_t i = 0; i < Node->mNumMeshes; i++)
		{
			auto l_MeshIndex = Node->mMeshes[i];
			auto l_AiMesh = Scene->mMeshes[l_MeshIndex];

			Log(Verbose, "Processing mesh: ", l_AiMesh->mName.C_Str());

			MeshComponent l_Mesh = {};
			AssimpMeshProcessor::CreateMeshComponent(Scene, BaseName, l_MeshIndex, l_Mesh);

			if (l_AiMesh->mMaterialIndex < Scene->mNumMaterials)
			{
				MaterialComponent l_Material = {};
				AssimpMaterialProcessor::CreateMaterialComponent(Scene->mMaterials[l_AiMesh->mMaterialIndex], BaseName, l_Material);
			}
		}
	}

	if (Node->mNumChildren)
	{
		for (uint32_t i = 0; i < Node->mNumChildren; i++)
		{
			ProcessAssimpNode(Node->mChildren[i], Scene, BaseName);
		}
	}
}
