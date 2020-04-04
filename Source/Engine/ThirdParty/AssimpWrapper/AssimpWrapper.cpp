#include "AssimpWrapper.h"

#include "stb/stb_image_write.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/DefaultLogger.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "../../Core/InnoLogger.h"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../../Core/IOService.h"
#include "../JSONWrapper/JSONWrapper.h"

namespace AssimpWrapper
{
	void processAssimpScene(json& j, const aiScene* scene, const char* exportName);
	void processAssimpNode(const std::function<void(json&, const aiNode*, const aiScene*, const char*)>& nodeFunctor, json& j, const aiNode* node, const aiScene* scene, const char* exportName);
	void processAssimpMesh(json& j, const aiScene* scene, const char* exportName, uint32_t meshIndex);
	size_t processMeshData(const aiMesh* mesh, const char* exportFileRelativePath);
	void processAssimpBone(json& j, const aiMesh* mesh);
	void processAssimpMaterial(json& j, const aiMaterial* material);
	void processTextureData(json& j, const char* fileName, TextureSampler sampler, TextureUsage usage, bool IsSRGB, uint32_t textureSlotIndex);
	void processAssimpAnimation(json& j, const aiAnimation* animation, const char* exportFileRelativePath);
	void decomposeTransformation(json& j, const aiNode* node);
};

bool AssimpWrapper::convertModel(const char* fileName, const char* exportPath)
{
	auto l_exportFileName = IOService::getFileName(fileName);
	auto l_exportFileRelativePath = exportPath + l_exportFileName + ".InnoModel";

	// Check if the file was converted already
	if (IOService::isFileExist(l_exportFileRelativePath.c_str()))
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", fileName, " has already been converted!");
		return true;
	}

	// read file via ASSIMP
	Assimp::Importer l_importer;
	const aiScene* l_scene;

	// Check if the file was exist
	if (IOService::isFileExist(fileName))
	{
		InnoLogger::Log(LogLevel::Verbose, "FileSystem: AssimpWrapper: converting ", fileName, "...");
#if defined _DEBUG
		std::string l_logFilePath = IOService::getWorkingDirectory() + "..//Res//Logs//AssimpLog_" + l_exportFileName + ".txt";
		Assimp::DefaultLogger::create(l_logFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif
		l_scene = l_importer.ReadFile(IOService::getWorkingDirectory() + fileName, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph | aiProcess_SplitLargeMeshes);
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: ", fileName, " doesn't exist!");
		return false;
	}
	if (l_scene)
	{
		if (l_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_scene->mRootNode)
		{
			InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: ", l_importer.GetErrorString());
			return false;
		}

		json j;

		processAssimpScene(j, l_scene, l_exportFileName.c_str());
		JSONWrapper::saveJsonDataToDisk(l_exportFileRelativePath.c_str(), j);

		InnoLogger::Log(LogLevel::Success, "FileSystem: AssimpWrapper: ", fileName, " has been converted.");
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: can't load file ", fileName, "!");
		return false;
	}

	return true;
}

void AssimpWrapper::processAssimpScene(json& j, const aiScene* scene, const char* exportName)
{
	auto l_timeData = g_pModuleManager->getTimeSystem()->getCurrentTime();
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

	if (scene->mNumAnimations)
	{
		for (uint32_t i = 0; i < scene->mNumAnimations; i++)
		{
			json j_child;

			auto l_validateFileName = IOService::validateFileName(scene->mAnimations[i]->mName.C_Str());
			auto l_animationFileName = "..//Res//ConvertedAssets//" + std::string(exportName) + "_" + l_validateFileName + ".InnoAnimation";
			processAssimpAnimation(j_child, scene->mAnimations[i], l_animationFileName.c_str());
			j["Animations"].emplace_back(l_animationFileName);
		}
	}

	auto f_getNodeTransform = [](json& j, const aiNode* node, const aiScene* scene, const char* exportName)
	{
		if (node->mName.C_Str())
		{
			json j_node;

			j_node["Name"] = node->mName.C_Str();
			decomposeTransformation(j_node, node);

			j["Nodes"].emplace_back(j_node);
		}
	};

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

	processAssimpNode(f_getNodeTransform, j, scene->mRootNode, scene, exportName);
	processAssimpNode(f_getMesh, j, scene->mRootNode, scene, exportName);

	// Assign bone ID to nodes transformation
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
	auto l_verticesNumber = mesh->mNumVertices;

	Array<Vertex> l_vertices;

	l_vertices.reserve(l_verticesNumber);
	l_vertices.fulfill();

	for (uint32_t i = 0; i < l_verticesNumber; i++)
	{
		Vertex l_Vertex;

		// positions
		if (&mesh->mVertices[i] != nullptr)
		{
			l_Vertex.m_pos.x = mesh->mVertices[i].x;
			l_Vertex.m_pos.y = mesh->mVertices[i].y;
			l_Vertex.m_pos.z = mesh->mVertices[i].z;
		}
		else
		{
			l_Vertex.m_pos.x = 0.0f;
			l_Vertex.m_pos.y = 0.0f;
			l_Vertex.m_pos.z = 0.0f;
		}

		// texture coordinates
		if (mesh->mTextureCoords[0])
		{
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			l_Vertex.m_texCoord.x = mesh->mTextureCoords[0][i].x;
			l_Vertex.m_texCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			l_Vertex.m_texCoord.x = 0.0f;
			l_Vertex.m_texCoord.y = 0.0f;
		}

		// normals
		if (mesh->mNormals)
		{
			l_Vertex.m_normal.x = mesh->mNormals[i].x;
			l_Vertex.m_normal.y = mesh->mNormals[i].y;
			l_Vertex.m_normal.z = mesh->mNormals[i].z;
		}
		else
		{
			l_Vertex.m_normal.x = 0.0f;
			l_Vertex.m_normal.y = 0.0f;
			l_Vertex.m_normal.z = 1.0f;
		}

		l_Vertex.m_pad1 = InnoMath::minVec2<float>;
		l_Vertex.m_pad2 = InnoMath::minVec4<float>;

		l_vertices[i] = l_Vertex;
	}

	Array<Index> l_indices;
	size_t l_indiceSize = 0;

	// bones weight
	if (mesh->mNumBones)
	{
		for (size_t i = 0; i < mesh->mNumBones; i++)
		{
			auto l_bone = mesh->mBones[i];
			if (l_bone->mNumWeights)
			{
				for (uint32_t j = 0; j < l_bone->mNumWeights; j++)
				{
					aiVertexWeight l_vertexWeight = l_bone->mWeights[j];
					auto l_Id = l_vertexWeight.mVertexId;
					auto l_weight = l_vertexWeight.mWeight;
					// Only the first 3 most weighted bones will be stored
					if (l_weight > l_vertices[l_Id].m_pad1.y)
					{
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
						// Old 2nd to 3rd
						l_vertices[l_Id].m_pad2.z = l_vertices[l_Id].m_pad2.x;
						l_vertices[l_Id].m_pad2.w = l_vertices[l_Id].m_pad2.y;
						// New as 2nd
						l_vertices[l_Id].m_pad2.x = (float)i;
						l_vertices[l_Id].m_pad2.y = l_weight;
					}
					else if (l_weight > l_vertices[l_Id].m_pad2.w)
					{
						// New as 3rd
						l_vertices[l_Id].m_pad2.z = (float)i;
						l_vertices[l_Id].m_pad2.w = l_weight;
					}
				}
			}
		}

		l_indices.reserve(l_verticesNumber);
		l_indices.fulfill();

		for (uint32_t i = 0; i < l_verticesNumber; i++)
		{
			l_indices[i] = i;
		}

		l_indiceSize = l_verticesNumber;
	}
	else
	{
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
			l_indices.reserve(l_verticesNumber);
			l_indices.fulfill();

			for (uint32_t i = 0; i < l_verticesNumber; i++)
			{
				l_indices[i] = i;
			}

			l_indiceSize = l_verticesNumber;
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

		aiQuaternion l_aiRot;
		aiVector3D l_aiPos;
		l_bone->mOffsetMatrix.DecomposeNoScaling(l_aiRot, l_aiPos);
		auto l_rot = Vec4(l_aiRot.x, l_aiRot.y, l_aiRot.z, l_aiRot.w);
		auto l_pos = Vec4(l_aiPos.x, l_aiPos.y, l_aiPos.z, 1.0f);

		JSONWrapper::to_json(j_child["Rotation"], l_rot);
		JSONWrapper::to_json(j_child["Position"], l_pos);

		j["Bones"].emplace_back(j_child);
	}
}

/*
aiTextureType::aiTextureType_NORMALS TextureUsage::NORMAL map_Kn normal map texture
aiTextureType::aiTextureType_DIFFUSE TextureUsage::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR TextureUsage::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_AMBIENT TextureUsage::ROUGHNESS map_Ka roughness texture
aiTextureType::aiTextureType_EMISSIVE TextureUsage::AMBIENT_OCCLUSION map_emissive AO texture
aiTextureType::AI_MATKEY_COLOR_DIFFUSE Kd Albedo RGB
aiTextureType::AI_MATKEY_COLOR_TRANSPARENT Tf Alpha A
aiTextureType::AI_MATKEY_COLOR_SPECULAR Ks Metallic
aiTextureType::AI_MATKEY_COLOR_AMBIENT Ka Roughness
aiTextureType::AI_MATKEY_COLOR_EMISSIVE Ke AO
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
				InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", l_AssString.C_Str(), " is unknown texture type!");
				break;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
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
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, false, 3);
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				processTextureData(j_child, l_localPath, TextureSampler::Sampler2D, TextureUsage::Sample, false, 4);
			}
			else
			{
				InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", l_AssString.C_Str(), " is unsupported texture type!");
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
	}
	else
	{
		j["Albedo"]["A"] = 1.0f;
	}
	if (material->Get(AI_MATKEY_COLOR_SPECULAR, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Metallic"] = l_result.r;
	}
	else
	{
		j["Metallic"] = 0.5f;
	}
	if (material->Get(AI_MATKEY_COLOR_AMBIENT, l_result) == aiReturn::aiReturn_SUCCESS)
	{
		j["Roughness"] = l_result.r;
	}
	else
	{
		j["Roughness"] = 0.5f;
	}
	if (material->Get(AI_MATKEY_COLOR_EMISSIVE, l_result) == aiReturn::aiReturn_SUCCESS)
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

/*
Binary data type:
Duration:float
NumChannels:uint32_t
KeyOffsets:uint32_t
NumKeys:uint32_t
Key:Key(Vec4+Vec4)

Binary data structure:
|Duration
|NumChannels
|KeyOffsets and NumKeys
	|Channel1
	|Channel2
	|...
	|ChannelN
|Channel1
	|Key1
	|Key2
	|...
	|KeyN
|Channel2
|...
|ChannelN
*/

void AssimpWrapper::processAssimpAnimation(json& j, const aiAnimation* aiAnimation, const char* exportFileRelativePath)
{
	std::ofstream l_file(IOService::getWorkingDirectory() + exportFileRelativePath, std::ios::out | std::ios::trunc | std::ios::binary);

	std::vector<Vec4> l_keyData;

	float l_duration = (float)aiAnimation->mDuration;
	IOService::serialize(l_file, &l_duration);

	uint32_t l_numChannels = aiAnimation->mNumChannels;
	if (l_numChannels)
	{
		IOService::serialize(l_file, &l_numChannels);

		uint32_t l_keyOffset = 0;

		for (uint32_t i = 0; i < l_numChannels; i++)
		{
			auto l_channel = aiAnimation->mChannels[i];

			if (l_channel->mNumPositionKeys != l_channel->mNumRotationKeys)
			{
				InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: Position key number is different than rotation key number in node: ", l_channel->mNodeName.C_Str(), "!");
				l_file.close();
				return;
			}

			uint32_t l_numKeys = l_channel->mNumPositionKeys;
			IOService::serialize(l_file, &l_keyOffset);
			IOService::serialize(l_file, &l_numKeys);
			l_keyOffset += l_numKeys;

			// Position-xyz, time-w, rotation-xyzw
			for (uint32_t j = 0; j < l_channel->mNumPositionKeys; j++)
			{
				auto l_posKey = l_channel->mPositionKeys[j];
				auto l_posKeyTime = l_posKey.mTime;
				auto l_posKeyValue = l_posKey.mValue;
				auto l_pos = Vec4(l_posKeyValue.x, l_posKeyValue.y, l_posKeyValue.z, (float)l_posKeyTime);

				l_keyData.emplace_back(l_pos);

				auto l_rotKey = l_channel->mRotationKeys[j];
				auto l_rotKeyValue = l_rotKey.mValue;
				auto l_rot = Vec4(l_rotKeyValue.x, l_rotKeyValue.y, l_rotKeyValue.z, l_rotKeyValue.w);

				l_keyData.emplace_back(l_rot);
			}
		}
	}

	IOService::serializeVector(l_file, l_keyData);

	l_file.close();
}

void AssimpWrapper::decomposeTransformation(json& j, const aiNode* node)
{
	// @TODO: optimize
	aiMatrix4x4 t = node->mTransformation;

	aiNode* l_parent = node->mParent;
	while (l_parent != nullptr)
	{
		t *= l_parent->mTransformation;
		l_parent = l_parent->mParent;
	}

	aiQuaternion l_aiRot;
	aiVector3D l_aiPos;
	t.DecomposeNoScaling(l_aiRot, l_aiPos);
	auto l_rot = Vec4(l_aiRot.x, l_aiRot.y, l_aiRot.z, l_aiRot.w);
	auto l_pos = Vec4(l_aiPos.x, l_aiPos.y, l_aiPos.z, 1.0f);

	JSONWrapper::to_json(j["Rotation"], l_rot);
	JSONWrapper::to_json(j["Position"], l_pos);
}