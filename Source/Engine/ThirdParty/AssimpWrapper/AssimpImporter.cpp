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
#include "../JSONWrapper/JSONWrapper.h"
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

	auto l_FullPath = g_Engine->Get<IOService>()->getWorkingDirectory() + FileName;

	Log(Verbose, "Converting ", l_FullPath.c_str(), "...");
#if defined INNO_DEBUG
	std::string l_LogFilePath = "AssimpLog_" + l_ExportFileName + ".txt";
	Assimp::DefaultLogger::create(l_LogFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif

	Assimp::Importer l_Importer;
	const aiScene* l_Scene = l_Importer.ReadFile(l_FullPath.c_str(),
		aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_CalcTangentSpace
		| aiProcess_FlipUVs
		| aiProcess_JoinIdenticalVertices
		| aiProcess_SplitLargeMeshes
		//| aiProcess_FindInstances // Do not merge instances so the culling result could be more optimized
		| aiProcess_OptimizeMeshes
		| aiProcess_OptimizeGraph
		| aiProcess_PreTransformVertices // Bake per-node transforms into vertex positions; required for glTF models where orientation is encoded in node transforms (e.g. NewSponza curtains)
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

	auto l_ModelBaseDir = g_Engine->Get<IOService>()->getFilePath(FileName);
	ProcessAssimpScene(l_Scene, l_ExportFileName.c_str(), l_ModelBaseDir.c_str());

	Log(Success, FileName, " has been imported.");
	return true;
}

void AssimpImporter::ProcessAssimpScene(const aiScene* Scene, const char* ExportName, const char* ModelBaseDir)
{
	Log(Verbose, "Processing scene: ", ExportName);

	// Collect mesh→material pairs during node processing
	std::vector<std::pair<std::string, std::string>> l_DrawCalls;
	ProcessAssimpNode(Scene->mRootNode, Scene, ExportName, ModelBaseDir, l_DrawCalls);

	// Build a child .InnoScene with one entity per draw call
	JSONWrapper::SaveChildScene(ExportName, l_DrawCalls);
	Log(Success, "Saved child scene: Scenes/", ExportName, ".InnoScene with ", l_DrawCalls.size(), " entities.");
}

// AssimpWrapper is an offline asset converter (Baker tool).
// Components are populated from Assimp data and saved to disk by the processors.
// EntityRegistry entity creation happens at runtime load time in AssetService.
void AssimpImporter::ProcessAssimpNode(const aiNode* Node, const aiScene* Scene, const char* BaseName, const char* ModelBaseDir,
	std::vector<std::pair<std::string, std::string>>& drawCalls)
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

			std::string l_MaterialName;
			if (l_AiMesh->mMaterialIndex < Scene->mNumMaterials)
			{
				MaterialComponent l_Material = {};
				AssimpMaterialProcessor::CreateMaterialComponent(Scene->mMaterials[l_AiMesh->mMaterialIndex], BaseName, ModelBaseDir, l_Material);
				l_MaterialName = l_Material.m_InstanceName.c_str();
			}

			drawCalls.push_back({l_Mesh.m_InstanceName.c_str(), l_MaterialName});
		}
	}

	if (Node->mNumChildren)
	{
		for (uint32_t i = 0; i < Node->mNumChildren; i++)
		{
			ProcessAssimpNode(Node->mChildren[i], Scene, BaseName, ModelBaseDir, drawCalls);
		}
	}
}
