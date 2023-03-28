#include "AssimpWrapper.h"

#include "stb_image_write.h"

#include "Importer.hpp"
#include "Exporter.hpp"
#include "DefaultLogger.hpp"
#include "scene.h"
#include "postprocess.h"
#include "../../Core/InnoLogger.h"

#include "../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

#include "../../Core/IOService.h"
#include "../JSONWrapper/JSONWrapper.h"

namespace Inno
{
	namespace AssimpWrapper
	{
		void to_json(json& j, const aiMatrix4x4& m);
		void from_json(const json& j, aiMatrix4x4& m);

		void processAssimpScene(json& j, const aiScene* scene, const char* exportName);
		void processAssimpNode(const std::function<void(json&, const aiNode*, const aiScene*, const char*)>& nodeFunctor, json& j, const aiNode* node, const aiScene* scene, const char* exportName);
		void processAssimpMesh(json& j, const aiScene* scene, const char* exportName, uint32_t meshIndex);
		size_t processMeshData(const aiMesh* mesh, const char* exportFileRelativePath);
		void processAssimpBone(json& j, const aiMesh* mesh);
		void processAssimpMaterial(json& j, const aiMaterial* material);
		void processTextureData(json& j, const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex);
		void processAssimpAnimation(json& j, const aiScene* scene, const aiAnimation* animation, const std::unordered_map<std::string, uint32_t>& boneNameIDMap, const std::unordered_map<std::string, Mat4>& boneNameOffsetMap, const char* exportFileRelativePath);
		void mergeTransformation(json& j, const aiNode* node);
		void decomposeTransformation(json& j, const aiMatrix4x4& m);
	};
}

void AssimpWrapper::to_json(json& j, const aiMatrix4x4& m)
{
	j["00"] = m.a1;
	j["01"] = m.a2;
	j["02"] = m.a3;
	j["03"] = m.a4;
	j["10"] = m.b1;
	j["11"] = m.b2;
	j["12"] = m.b3;
	j["13"] = m.b4;
	j["20"] = m.c1;
	j["21"] = m.c2;
	j["22"] = m.c3;
	j["23"] = m.c4;
	j["30"] = m.d1;
	j["31"] = m.d2;
	j["32"] = m.d3;
	j["33"] = m.d4;
}

void AssimpWrapper::from_json(const json& j, aiMatrix4x4& m)
{
	m.a1 = j["00"];
	m.a2 = j["01"];
	m.a3 = j["02"];
	m.a4 = j["03"];
	m.b1 = j["10"];
	m.b2 = j["11"];
	m.b3 = j["12"];
	m.b4 = j["13"];
	m.c1 = j["20"];
	m.c2 = j["21"];
	m.c3 = j["22"];
	m.c4 = j["23"];
	m.d1 = j["30"];
	m.d2 = j["31"];
	m.d3 = j["32"];
	m.d4 = j["33"];
}

bool AssimpWrapper::convertModel(const char* fileName, const char* exportPath)
{
	auto l_exportFileName = IOService::getFileName(fileName);
	auto l_exportFileRelativePath = exportPath + l_exportFileName + ".InnoModel";

	// Check if the file was converted already
	if (IOService::isFileExist(l_exportFileRelativePath.c_str()))
	{
		InnoLogger::Log(LogLevel::Warning, "AssimpWrapper: ", fileName, " has already been converted!");
		return true;
	}

	// read file via ASSIMP
	Assimp::Importer l_importer;
	const aiScene* l_scene;

	// Check if the file was exist
	if (IOService::isFileExist(fileName))
	{
		InnoLogger::Log(LogLevel::Verbose, "AssimpWrapper: Converting ", fileName, "...");
#if defined INNO_DEBUG
		std::string l_logFilePath = IOService::getWorkingDirectory() + "..//Res//Logs//AssimpLog_" + l_exportFileName + ".txt";
		Assimp::DefaultLogger::create(l_logFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif
		l_scene = l_importer.ReadFile(IOService::getWorkingDirectory() + fileName, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph);
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "AssimpWrapper: ", fileName, " doesn't exist!");
		return false;
	}
	if (l_scene)
	{
		if (l_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_scene->mRootNode)
		{
			InnoLogger::Log(LogLevel::Error, "AssimpWrapper: ", l_importer.GetErrorString());
			return false;
		}

		json j;

		processAssimpScene(j, l_scene, l_exportFileName.c_str());
		JSONWrapper::saveJsonDataToDisk(l_exportFileRelativePath.c_str(), j);

		InnoLogger::Log(LogLevel::Success, "AssimpWrapper: ", fileName, " has been converted.");
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "AssimpWrapper: Can't load file ", fileName, "!");
		return false;
	}

	return true;
}

void AssimpWrapper::processAssimpScene(json& j, const aiScene* scene, const char* exportName)
{
	auto l_timeData = g_Engine->getTimeSystem()->getCurrentTime();
	auto l_timeDataStr =
		"["
		+ std::to_string(l_timeData.Year)
		+ "-" + std::to_string(l_timeData.Month)
		+ "-" + std::to_string(l_timeData.Day)
		+ "-" + std::to_string(l_timeData.Hour)
		+ "-" + std::to_string(l_timeData.Minute)
		+ "-" + std::to_string(l_timeData.Second)
		+ "-" + std::to_string(l_timeData.Millisecond)
		+ "]";

	j["Timestamp"] = l_timeDataStr;

	auto f_getMesh = [](json& j, const aiNode* node, const aiScene* scene, const char* exportName)
	{
		// process each mesh located at the current node
		if (node->mNumMeshes)
		{
			for (uint32_t i = 0; i < node->mNumMeshes; i++)
			{
				json j_child;

				processAssimpMesh(j_child, scene, exportName, node->mMeshes[i]);
				j["Meshes"].emplace_back(j_child);
			}
		}
	};

	InnoLogger::Log(LogLevel::Verbose, "AssimpWrapper: Converting meshes...");
	processAssimpNode(f_getMesh, j, scene->mRootNode, scene, exportName);

	InnoLogger::Log(LogLevel::Verbose, "AssimpWrapper: Assign transformation matrices to bones...");
	std::unordered_map<std::string, uint32_t> l_boneNameIDMap;
	std::unordered_map<std::string, Mat4> l_boneNameOffsetMap;

	for (auto& i : j["Meshes"])
	{
		for (auto& b : i["Bones"])
		{
			l_boneNameIDMap.emplace(b["Name"], b["ID"]);
			Mat4 l_m;
			JSONWrapper::from_json(b["Transformation"], l_m);
			l_boneNameOffsetMap.emplace(b["Name"], l_m);
		}
	}

	if (scene->mNumAnimations)
	{
		InnoLogger::Log(LogLevel::Verbose, "AssimpWrapper: Converting animations...");

		for (uint32_t i = 0; i < scene->mNumAnimations; i++)
		{
			json j_child;

			auto l_validateFileName = IOService::validateFileName(scene->mAnimations[i]->mName.C_Str());
			auto l_animationFileName = "..//Res//ConvertedAssets//" + std::string(exportName) + "_" + l_validateFileName + ".InnoAnimation";
			processAssimpAnimation(j_child, scene, scene->mAnimations[i], l_boneNameIDMap, l_boneNameOffsetMap, l_animationFileName.c_str());
			j["Animations"].emplace_back(j_child);
		}
	}
}

void AssimpWrapper::processAssimpNode(const std::function<void(json&, const aiNode*, const aiScene*, const char*)>& nodeFunctor, json& j, const aiNode* node, const aiScene* scene, const char* exportName)
{
	nodeFunctor(j, node, scene, exportName);

	// process children node
	if (node->mNumChildren)
	{
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			processAssimpNode(nodeFunctor, j, node->mChildren[i], scene, exportName);
		}
	}
}

void AssimpWrapper::processAssimpMesh(json& j, const aiScene* scene, const char* exportName, uint32_t meshIndex)
{
	auto l_mesh = scene->mMeshes[meshIndex];

	// process vertices and indices
	j["Name"] = l_mesh->mName.C_Str();
	j["VerticesNumber"] = l_mesh->mNumVertices;
	auto l_meshFileName = "..//Res//ConvertedAssets//" + std::string(exportName) + "_" + std::to_string(meshIndex) + ".InnoMesh";
	j["IndicesNumber"] = processMeshData(l_mesh, l_meshFileName.c_str());
	j["File"] = l_meshFileName;
	j["MeshSource"] = MeshSource::Customized;

	// process bones
	if (l_mesh->mNumBones)
	{
		processAssimpBone(j, l_mesh);
	}

	// process material
	if (l_mesh->mMaterialIndex)
	{
		processAssimpMaterial(j["Material"], scene->mMaterials[l_mesh->mMaterialIndex]);
	}
}

size_t AssimpWrapper::processMeshData(const aiMesh* mesh, const char* exportFileRelativePath)
{
	auto l_numVertices = mesh->mNumVertices;

	Array<Vertex> l_vertices;

	l_vertices.reserve(l_numVertices);
	l_vertices.fulfill();

	// positions
	for (uint32_t i = 0; i < l_numVertices; i++)
	{
		Vertex l_Vertex;

		l_vertices[i].m_pos.x = mesh->mVertices[i].x;
		l_vertices[i].m_pos.y = mesh->mVertices[i].y;
		l_vertices[i].m_pos.z = mesh->mVertices[i].z;
		l_vertices[i].m_pos.w = 1.0f;
		l_vertices[i].m_normal.x = 0.0f;
		l_vertices[i].m_normal.y = 0.0f;
		l_vertices[i].m_normal.z = 1.0f;
	}

	// texture coordinates
	if (mesh->mTextureCoords[0])
	{
		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			l_vertices[i].m_texCoord.x = mesh->mTextureCoords[0][i].x;
			l_vertices[i].m_texCoord.y = mesh->mTextureCoords[0][i].y;
		}
	}

	// normals
	if (mesh->mNormals)
	{
		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			l_vertices[i].m_normal.x = mesh->mNormals[i].x;
			l_vertices[i].m_normal.y = mesh->mNormals[i].y;
			l_vertices[i].m_normal.z = mesh->mNormals[i].z;
		}
	}

	Array<Index> l_indices;
	size_t l_indiceSize = 0;

	if (mesh->mNumFaces)
	{
		// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace l_face = mesh->mFaces[i];
			l_indiceSize += l_face.mNumIndices;
		}

		l_indices.reserve(l_indiceSize);
		l_indices.fulfill();

		uint32_t l_index = 0;
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace l_face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (uint32_t j = 0; j < l_face.mNumIndices; j++)
			{
				l_indices[l_index] = l_face.mIndices[j];
				l_index++;
			}
		}
	}
	else
	{
		l_indices.reserve(l_numVertices);
		l_indices.fulfill();

		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			l_indices[i] = i;
		}

		l_indiceSize = l_numVertices;
	}

	// bones weight

	for (uint32_t i = 0; i < mesh->mNumBones; i++)
	{
		auto l_bone = mesh->mBones[i];

		for (uint32_t j = 0; j < l_bone->mNumWeights; j++)
		{
			auto l_vertexWeight = l_bone->mWeights[j];
			auto l_Id = l_vertexWeight.mVertexId;
			auto l_weight = l_vertexWeight.mWeight;

			// Only the first 4 most weighted bones will be stored
			if (l_weight > l_vertices[l_Id].m_pad1.y)
			{
				// Old 3rd to 4th
				l_vertices[l_Id].m_pos.w = l_vertices[l_Id].m_pad2.z;
				l_vertices[l_Id].m_normal.w = l_vertices[l_Id].m_pad2.w;
				// Old 2nd to 3rd
				l_vertices[l_Id].m_pad2.z = l_vertices[l_Id].m_pad2.x;
				l_vertices[l_Id].m_pad2.w = l_vertices[l_Id].m_pad2.y;
				// Old 1st to 2nd
				l_vertices[l_Id].m_pad2.x = l_vertices[l_Id].m_pad1.x;
				l_vertices[l_Id].m_pad2.y = l_vertices[l_Id].m_pad1.y;
				// New as 1st
				l_vertices[l_Id].m_pad1.x = (float)i;
				l_vertices[l_Id].m_pad1.y = l_weight;
			}
			else if (l_weight > l_vertices[l_Id].m_pad2.y)
			{
				// Old 3rd to 4th
				l_vertices[l_Id].m_pos.w = l_vertices[l_Id].m_pad2.z;
				l_vertices[l_Id].m_normal.w = l_vertices[l_Id].m_pad2.w;
				// Old 2nd to 3rd
				l_vertices[l_Id].m_pad2.z = l_vertices[l_Id].m_pad2.x;
				l_vertices[l_Id].m_pad2.w = l_vertices[l_Id].m_pad2.y;
				// New as 2nd
				l_vertices[l_Id].m_pad2.x = (float)i;
				l_vertices[l_Id].m_pad2.y = l_weight;
			}
			else if (l_weight > l_vertices[l_Id].m_pad2.w)
			{
				// Old 3rd to 4th
				l_vertices[l_Id].m_pos.w = l_vertices[l_Id].m_pad2.z;
				l_vertices[l_Id].m_normal.w = l_vertices[l_Id].m_pad2.w;
				// New as 3rd
				l_vertices[l_Id].m_pad2.z = (float)i;
				l_vertices[l_Id].m_pad2.w = l_weight;
			}
			else if (l_weight > l_vertices[l_Id].m_normal.w)
			{
				// New as 4th
				l_vertices[l_Id].m_pos.w = (float)i;
				l_vertices[l_Id].m_normal.w = l_weight;
			}
		}
	}

	std::ofstream l_file(IOService::getWorkingDirectory() + exportFileRelativePath, std::ios::out | std::ios::trunc | std::ios::binary);

	IOService::serializeVector(l_file, l_vertices);
	IOService::serializeVector(l_file, l_indices);

	l_file.close();

	return l_indiceSize;
}

void AssimpWrapper::processAssimpBone(json& j, const aiMesh* mesh)
{
	for (uint32_t i = 0; i < mesh->mNumBones; i++)
	{
		json j_child;

		auto l_bone = mesh->mBones[i];

		j_child["Name"] = l_bone->mName.C_Str();
		j_child["ID"] = i;

		to_json(j_child["Transformation"], l_bone->mOffsetMatrix);

		j["Bones"].emplace_back(j_child);
	}
}

/*
aiTextureType::aiTextureType_HEIGHT TextureUsage::NORMAL map_Bump normal map texture
aiTextureType::aiTextureType_DIFFUSE TextureUsage::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR TextureUsage::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_SHININESS TextureUsage::ROUGHNESS map_Ns roughness texture
aiTextureType::aiTextureType_AMBIENT TextureUsage::AMBIENT_OCCLUSION map_Ka AO texture
aiTextureType::AI_MATKEY_COLOR_DIFFUSE Kd Albedo RGB
aiTextureType::AI_MATKEY_COLOR_TRANSPARENT Tf Alpha A
aiTextureType::AI_MATKEY_COLOR_SPECULAR Ks Metallic
aiTextureType::AI_MATKEY_SHININESS Ns Roughness
aiTextureType::AI_MATKEY_COLOR_AMBIENT Ka AO
aiTextureType::AI_MATKEY_COLOR_REFLECTIVE Thickness
*/

void AssimpWrapper::processAssimpMaterial(json& j, const aiMaterial* material)
{
	for (uint32_t i = 0; i < aiTextureType_UNKNOWN; i++)
	{
		if (material->GetTextureCount(aiTextureType(i)) > 0)
		{
			aiString l_AssString;
			material->GetTexture(aiTextureType(i), 0, &l_AssString);
			auto l_localPath = l_AssString.C_Str();
			json j_child;

			if (aiTextureType(i) == aiTextureType::aiTextureType_NONE)
			{
				InnoLogger::Log(LogLevel::Warning, "AssimpWrapper: ", l_AssString.C_Str(), " is unknown texture type!");
				break;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_HEIGHT)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, false, 0);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, true, 1);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, false, 2);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SHININESS)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, false, 3);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, false, 4);
			}
			else
			{
				InnoLogger::Log(LogLevel::Warning, "AssimpWrapper: ", l_AssString.C_Str(), " is unsupported texture type!");
				break;
			}
			j["Textures"].emplace_back(j_child);
		}
	}

	auto l_result = aiColor3D();

	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Albedo"] =
		{
			{"R", l_result.r},
			{"G", l_result.g},
			{"B", l_result.b},
		};
	}
	else
	{
		j["Albedo"] =
		{
			{"R", 1.0f},
			{"G", 1.0f},
			{"B", 1.0f},
		};
	}
	if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Albedo"]["A"] = l_result.r;
		if (l_result.r == 1.0f)
		{
			j["ShaderModel"] = ShaderModel::Opaque;
		}
		else
		{
			j["ShaderModel"] = ShaderModel::Transparent;
		}
	}
	else
	{
		j["Albedo"]["A"] = 1.0f;
		j["ShaderModel"] = ShaderModel::Opaque;
	}
	if (material->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Metallic"] = l_result.r;
	}
	else
	{
		j["Metallic"] = 0.5f;
	}
	if (material->Get(AI_MATKEY_SHININESS, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Roughness"] = l_result.r;
	}
	else
	{
		j["Roughness"] = 0.5f;
	}
	if (material->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["AO"] = l_result.r;
	}
	else
	{
		j["AO"] = 1.0f;
	}
	if (material->Get(AI_MATKEY_COLOR_REFLECTIVE, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Thickness"] = l_result.r;
	}
	else
	{
		j["Thickness"] = 1.0f;
	}
}

void AssimpWrapper::processTextureData(json& j, const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex)
{
	j["Sampler"] = sampler;
	j["Usage"] = usage;
	j["IsSRGB"] = IsSRGB;
	j["TextureSlotIndex"] = textureSlotIndex;
	j["File"] = fileName;
}

aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const char* name)
{
	for (uint32_t i = 0; i < pAnimation->mNumChannels; i++)
	{
		auto pNodeAnim = pAnimation->mChannels[i];

		if (!strcmp(pNodeAnim->mNodeName.C_Str(), name))
		{
			return pNodeAnim;
		}
	}

	return nullptr;
}

uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumScalingKeys > 0);

	for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);
}

uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumRotationKeys > 0);

	for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);
}

uint32_t FindTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumPositionKeys > 0);

	for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
		{
			return i;
		}
	}

	assert(0);
}

aiVector3D CalcInterpolatedScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1)
	{
		return pNodeAnim->mScalingKeys[0].mValue;
	}

	auto ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
	auto NextScalingIndex = (ScalingIndex + 1);

	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);

	float DeltaTime = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);

	const aiVector3D& StartScalingQ = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& EndScalingQ = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;

	aiVector3D l_result;
	Assimp::Interpolator<aiVector3D> l_lerp;
	l_lerp(l_result, StartScalingQ, EndScalingQ, Factor);

	return l_result;
}

aiQuaternion CalcInterpolatedRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumRotationKeys == 1)
	{
		return pNodeAnim->mRotationKeys[0].mValue;
	}

	auto RotationIndex = FindRotation(AnimationTime, pNodeAnim);
	auto NextRotationIndex = (RotationIndex + 1);

	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);

	float DeltaTime = (float)pNodeAnim->mRotationKeys[NextRotationIndex].mTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);

	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;

	aiQuaternion l_result;
	aiQuaternion::Interpolate(l_result, StartRotationQ, EndRotationQ, Factor);
	l_result = l_result.Normalize();

	return l_result;
}

aiVector3D CalcInterpolatedTranslation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1)
	{
		return pNodeAnim->mPositionKeys[0].mValue;
	}

	auto TranslationIndex = FindTranslation(AnimationTime, pNodeAnim);
	auto NextTranslationIndex = (TranslationIndex + 1);

	assert(NextTranslationIndex < pNodeAnim->mNumPositionKeys);

	float DeltaTime = (float)pNodeAnim->mPositionKeys[NextTranslationIndex].mTime - (float)pNodeAnim->mPositionKeys[TranslationIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[TranslationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);

	const aiVector3D& StartTranslationQ = pNodeAnim->mPositionKeys[TranslationIndex].mValue;
	const aiVector3D& EndTranslationQ = pNodeAnim->mPositionKeys[NextTranslationIndex].mValue;

	aiVector3D l_result;
	Assimp::Interpolator<aiVector3D> l_lerp;
	l_lerp(l_result, StartTranslationQ, EndTranslationQ, Factor);

	return l_result;
}

void ReadNodeHeirarchy(float AnimationTime, const aiAnimation* pAnimation, const aiNode* pNode, const Mat4& ParentTransform, std::vector<Mat4>& transforms, const std::unordered_map<std::string, uint32_t>& boneNameIDMap, const std::unordered_map<std::string, Mat4>& boneNameOffsetMap)
{
	Mat4 l_lm;

	std::memcpy(&l_lm, &pNode->mTransformation, sizeof(Mat4));

	auto pNodeAnim = FindNodeAnim(pAnimation, pNode->mName.C_Str());

	if (pNodeAnim)
	{
		auto l_aiS = CalcInterpolatedScaling(AnimationTime, pNodeAnim);
		auto l_s = Vec4(l_aiS.x, l_aiS.y, l_aiS.z, 1.0f);
		auto l_sM = InnoMath::toScaleMatrix(l_s);

		auto l_aiR = CalcInterpolatedRotation(AnimationTime, pNodeAnim);
		auto l_r = Vec4(l_aiR.x, l_aiR.y, l_aiR.z, l_aiR.w);
		auto l_rM = InnoMath::toRotationMatrix(l_r);

		auto l_aiT = CalcInterpolatedTranslation(AnimationTime, pNodeAnim);
		auto l_t = Vec4(l_aiT.x, l_aiT.y, l_aiT.z, 1.0f);
		auto l_tM = InnoMath::toTranslationMatrix(l_t);

		l_lm = l_tM * l_rM * l_sM;
	}

	auto l_gm = ParentTransform * l_lm;

	auto l_boneOffset = boneNameOffsetMap.find(pNode->mName.C_Str());
	if (l_boneOffset != boneNameOffsetMap.end())
	{
		auto l_final = l_gm * l_boneOffset->second;
		auto l_ID = boneNameIDMap.find(pNode->mName.C_Str());
		if (l_ID != boneNameIDMap.end())
		{
			transforms[l_ID->second] = l_final;
		}
	}

	for (uint32_t i = 0; i < pNode->mNumChildren; i++)
	{
		ReadNodeHeirarchy(AnimationTime, pAnimation, pNode->mChildren[i], l_gm, transforms, boneNameIDMap, boneNameOffsetMap);
	}
}

/*
Binary data type:
Duration:float
NumChannels:uint32_t
NumTicks:uint32_t
Key:Key(Mat4)

Binary data structure:
|Duration
|NumChannels
|NumTicks
|Tick1
	|Channel1
	|Channel2
	|...
	|ChannelN
|Tick2
|...
|TickN
*/
void AssimpWrapper::processAssimpAnimation(json& j, const aiScene* scene, const aiAnimation* animation, const std::unordered_map<std::string, uint32_t>& boneNameIDMap, const std::unordered_map<std::string, Mat4>& boneNameOffsetMap, const char* exportFileRelativePath)
{
	uint32_t l_numChannels = animation->mNumChannels;
	if (l_numChannels == 0)
	{
		return;
	}

	std::ofstream l_file(IOService::getWorkingDirectory() + exportFileRelativePath, std::ios::out | std::ios::trunc | std::ios::binary);
	j["File"] = exportFileRelativePath;

	auto l_duration = (float)animation->mDuration;
	IOService::serialize(l_file, &l_duration);

	auto l_validNumChannels = (uint32_t)boneNameIDMap.size();
	IOService::serialize(l_file, &l_validNumChannels);

	float l_ticksPerSecond = (float)animation->mTicksPerSecond != 0.0f ? (float)animation->mTicksPerSecond : 30.0f;
	auto l_timeInTicks = 1.0f / l_ticksPerSecond;
	auto l_numTicks = l_duration / l_timeInTicks;
	auto l_numTicksInt = (uint32_t)std::ceil(l_numTicks);

	IOService::serialize(l_file, &l_numTicksInt);

	std::vector<Mat4> l_transforms;
	auto l_numTransforms = (size_t)l_numTicksInt * l_validNumChannels;
	l_transforms.reserve(l_numTransforms);

	for (uint32_t i = 0; i < l_numTicksInt; i++)
	{
		auto l_m = InnoMath::generateIdentityMatrix<float>();
		float l_animationTime = (float)i * l_timeInTicks;

		std::vector<Mat4> l_transformsInCurrentTick;
		l_transformsInCurrentTick.resize(l_validNumChannels);

		ReadNodeHeirarchy(l_animationTime, animation, scene->mRootNode, l_m, l_transformsInCurrentTick, boneNameIDMap, boneNameOffsetMap);

		l_transforms.insert(l_transforms.end(), l_transformsInCurrentTick.begin(), l_transformsInCurrentTick.end());
	}

	IOService::serializeVector(l_file, l_transforms);

	l_file.close();
}