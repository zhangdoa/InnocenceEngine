#include "AssimpImporter.h"
#include "../../Common/Array.h"
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
#include "../../Common/TaskScheduler.h"
#include "../../Services/AssetService.h"
#include "../JSONWrapper/JSONWrapper.h"
#include "../../Engine.h"

using namespace Inno;

bool AssimpImporter::Import(const char* FileName)
{
	auto l_ExportFileName = g_Engine->Get<IOService>()->GetFileName(FileName);
	if (!g_Engine->Get<IOService>()->IsFileExist(FileName))
	{
		Log(Error, "", FileName, " doesn't exist!");
		return false;
	}

	auto l_FullPath = g_Engine->Get<IOService>()->GetWorkingDirectory() + FileName;

	Log(Verbose, "Converting ", l_FullPath.c_str(), "...");
#if defined INNO_DEBUG
	std::string l_LogFilePath = "AssimpLog_" + l_ExportFileName + ".txt";
	Assimp::DefaultLogger::create(l_LogFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif

	Assimp::Importer l_Importer;
	// aiProcess_OptimizeMeshes is intentionally OFF: together with PreTransformVertices
	// it merges meshes that differ only by material into a single mesh, losing the
	// per-material split. Concrete symptom on Sponza (glTF): the whole main model
	// plus curtains collapse down to ~15 meshes all referencing material 0, so at
	// runtime every surface shows whichever material happened to sort first
	// (e.g. curtains rendered with pillar/ground texture).
	const aiScene* l_Scene = l_Importer.ReadFile(l_FullPath.c_str(),
		aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_CalcTangentSpace
		| aiProcess_FlipUVs
		| aiProcess_JoinIdenticalVertices
		| aiProcess_SplitLargeMeshes
		//| aiProcess_FindInstances // Do not merge instances so the culling result could be more optimized
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

	auto l_ModelBaseDir = g_Engine->Get<IOService>()->GetFilePath(FileName);
	ProcessAssimpScene(l_Scene, l_ExportFileName.c_str(), l_ModelBaseDir.c_str());

	Log(Success, FileName, " has been imported.");
	return true;
}

namespace
{
	std::string MakeMeshInstanceName(uint32_t meshIndex, const char* baseName)
	{
		return std::string(baseName) + "." + std::to_string(meshIndex) + ".MeshComponent";
	}

	std::string MakeMaterialInstanceName(const aiMaterial* material, uint32_t materialIndex, const char* baseName)
	{
		// Mirror CreateMaterialComponent's naming: aiString returned by value,
		// length check to synthesize a name for unnamed glTF materials.
		aiString l_AiName = material->GetName();
		std::string l_Name;
		if (l_AiName.length == 0)
			l_Name = "material_" + std::to_string(materialIndex);
		else
			l_Name.assign(l_AiName.C_Str(), l_AiName.length);
		return std::string(baseName) + "." + l_Name + ".MaterialComponent";
	}

	void CollectAssimpWork(const aiNode* node, const aiScene* scene, const char* baseName,
		std::unordered_set<uint32_t>& outMeshIndices,
		std::unordered_set<uint32_t>& outMaterialIndices,
		Inno::Array<std::pair<std::string, std::string>>& outDrawCalls)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			uint32_t l_MeshIndex = node->mMeshes[i];
			outMeshIndices.insert(l_MeshIndex);

			const aiMesh* l_AiMesh = scene->mMeshes[l_MeshIndex];
			std::string l_MeshName = MakeMeshInstanceName(l_MeshIndex, baseName);

			std::string l_MaterialName;
			if (l_AiMesh->mMaterialIndex < scene->mNumMaterials)
			{
				outMaterialIndices.insert(l_AiMesh->mMaterialIndex);
				l_MaterialName = MakeMaterialInstanceName(scene->mMaterials[l_AiMesh->mMaterialIndex], l_AiMesh->mMaterialIndex, baseName);
			}

			outDrawCalls.push_back({ std::move(l_MeshName), std::move(l_MaterialName) });
		}

		for (uint32_t i = 0; i < node->mNumChildren; i++)
			CollectAssimpWork(node->mChildren[i], scene, baseName, outMeshIndices, outMaterialIndices, outDrawCalls);
	}
}

// AssimpWrapper is an offline asset converter (Baker tool).
// Components are populated from Assimp data and saved to disk by the processors.
// EntityRegistry entity creation happens at runtime load time in AssetService.
//
// Fan-out: mesh serialization, material JSON save, and BC texture compression
// are independent per-asset — submit one TaskScheduler task per unique mesh
// and per unique material, wait on all before writing the child scene JSON
// (which needs the full mesh→material draw-call table). Texture dedup across
// materials is handled inside AssetService::ImportTexture.
void AssimpImporter::ProcessAssimpScene(const aiScene* Scene, const char* ExportName, const char* ModelBaseDir)
{
	Log(Verbose, "Processing scene: ", ExportName);

	std::unordered_set<uint32_t> l_UniqueMeshes;
	std::unordered_set<uint32_t> l_UniqueMaterials;
	Inno::Array<std::pair<std::string, std::string>> l_DrawCalls;
	CollectAssimpWork(Scene->mRootNode, Scene, ExportName, l_UniqueMeshes, l_UniqueMaterials, l_DrawCalls);

	auto* l_Scheduler = g_Engine->Get<TaskScheduler>();
	Inno::Array<SharedPtr<ITask>> l_TaskHandles;
	l_TaskHandles.reserve(l_UniqueMeshes.size() + l_UniqueMaterials.size());

	// Copy C-strings into std::string so task lambdas own their storage; the
	// aiScene pointer stays valid for the lifetime of the outer Assimp::Importer
	// on the caller stack, and we wait on all handles before returning.
	std::string l_BaseName(ExportName);
	std::string l_ModelBaseDir(ModelBaseDir);

	for (uint32_t l_MeshIndex : l_UniqueMeshes)
	{
		auto l_Task = l_Scheduler->Submit(ITask::Desc("AssimpImport_Mesh", ITask::Type::Once),
			[Scene, l_BaseName, l_MeshIndex]()
			{
				MeshComponent l_Mesh = {};
				AssimpMeshProcessor::CreateMeshComponent(Scene, l_BaseName.c_str(), l_MeshIndex, l_Mesh);
			});
		l_Task->Activate();
		l_TaskHandles.push_back(l_Task);
	}

	for (uint32_t l_MatIndex : l_UniqueMaterials)
	{
		auto l_Task = l_Scheduler->Submit(ITask::Desc("AssimpImport_Material", ITask::Type::Once),
			[Scene, l_BaseName, l_ModelBaseDir, l_MatIndex]()
			{
				MaterialComponent l_Material = {};
				AssimpMaterialProcessor::CreateMaterialComponent(Scene->mMaterials[l_MatIndex], l_MatIndex, l_BaseName.c_str(), l_ModelBaseDir.c_str(), l_Material);
			});
		l_Task->Activate();
		l_TaskHandles.push_back(l_Task);
	}

	for (auto& l_Handle : l_TaskHandles)
		l_Handle->Wait();

	JSONWrapper::SaveChildScene(ExportName, l_DrawCalls);
	Log(Success, "Saved child scene: Scenes/", ExportName, ".InnoScene with ", l_DrawCalls.size(), " entities.");
}
