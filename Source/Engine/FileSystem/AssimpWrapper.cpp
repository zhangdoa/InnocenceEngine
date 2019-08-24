#include "AssimpWrapper.h"

#include "stb/stb_image_write.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/DefaultLogger.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "../Core/InnoLogger.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

#include "../Core/IOService.h"
#include "JSONParser.h"

namespace InnoFileSystemNS
{
	namespace AssimpWrapper
	{
		json processAssimpScene(const aiScene* aiScene, const std::string& exportName);
		bool processAssimpNode(json& j, const aiNode * node, const aiScene * scene, const std::string& exportName);
		json processAssimpMesh(const aiScene * scene, const std::string& exportName, unsigned int meshIndex);
		size_t processMeshData(const aiMesh * aiMesh, const std::string& exportFileRelativePath);
		void processAssimpBone(const aiMesh * aiMesh, const std::string& exportFileRelativePath);
		void processAssimpMaterial(const aiMaterial * aiMaterial, const std::string& exportFileRelativePath);
		json processTextureData(const std::string & fileName, TextureSamplerType samplerType, TextureUsageType usageType);
		void processAssimpAnimation(const aiAnimation * aiAnimation, const std::string& exportFileRelativePath);
	};
}

bool InnoFileSystemNS::AssimpWrapper::convertModel(const std::string & fileName, const std::string & exportPath)
{
	auto l_exportFileName = IOService::getFileName(fileName);
	auto l_exportFileRelativePath = exportPath + l_exportFileName + ".InnoModel";

	// Check if the file was converted already
	if (IOService::isFileExist(l_exportFileRelativePath))
	{
		InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", fileName.c_str(), " has already been converted!");
		return true;
	}

	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;

	// Check if the file was exist
	if (IOService::isFileExist(fileName))
	{
		InnoLogger::Log(LogLevel::Verbose, "FileSystem: AssimpWrapper: converting ", fileName.c_str(), "...");
#if defined _DEBUG
		std::string l_logFilePath = IOService::getWorkingDirectory() + "res/log/AssimpLog" + fileName + ".txt";
		Assimp::DefaultLogger::create(l_logFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif
		l_assScene = l_assImporter.ReadFile(IOService::getWorkingDirectory() + fileName, aiProcess_Triangulate | aiProcess_FlipUVs);
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: ", fileName.c_str(), " doesn't exist!");
		return false;
	}
	if (l_assScene)
	{
		if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
		{
			InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: ", l_assImporter.GetErrorString());
			return false;
		}
		auto l_result = processAssimpScene(l_assScene, l_exportFileName);
		JSONParser::saveJsonDataToDisk(l_exportFileRelativePath, l_result);

		InnoLogger::Log(LogLevel::Success, "FileSystem: AssimpWrapper: ", fileName.c_str(), " has been converted.");
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: can't load file ", fileName.c_str(), "!");
		return false;
	}

	return true;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpScene(const aiScene* aiScene, const std::string& exportName)
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

	json j;

	j["Timestamp"] = l_timeDataStr;

	if (aiScene->mNumAnimations)
	{
		for (unsigned int i = 0; i < aiScene->mNumAnimations; i++)
		{
			auto l_animationFileName = "//Res//ConvertedAssets//" + exportName + "_" + std::to_string(i) + ".InnoAnimation";
			processAssimpAnimation(aiScene->mAnimations[i], l_animationFileName);
			j["AnimationFiles"].emplace_back(l_animationFileName);
		}

		auto l_rootTransformationMat = aiScene->mRootNode->mTransformation;
		aiQuaternion l_aiRot;
		aiVector3D l_aiPos;
		l_rootTransformationMat.DecomposeNoScaling(l_aiRot, l_aiPos);
		auto l_rot = Vec4(l_aiRot.x, l_aiRot.y, l_aiRot.z, l_aiRot.w);
		auto l_pos = Vec4(l_aiPos.x, l_aiPos.y, l_aiPos.z, 1.0f);

		JSONParser::to_json(j["RootOffsetRotation"], l_rot);
		JSONParser::to_json(j["RootOffsetPosition"], l_pos);
	}

	processAssimpNode(j, aiScene->mRootNode, aiScene, exportName);

	return j;
}

bool InnoFileSystemNS::AssimpWrapper::processAssimpNode(json& j, const aiNode * node, const aiScene * scene, const std::string& exportName)
{
	// process each mesh located at the current node
	if (node->mNumMeshes)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			j["Meshes"].emplace_back(processAssimpMesh(scene, exportName, node->mMeshes[i]));
		}
	}

	// process children node
	if (node->mNumChildren)
	{
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processAssimpNode(j, node->mChildren[i], scene, exportName);
		}
	}

	return true;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpMesh(const aiScene * scene, const std::string& exportName, unsigned int meshIndex)
{
	json l_meshData;

	auto l_aiMesh = scene->mMeshes[meshIndex];

	// process vertices and indices
	l_meshData["MeshName"] = *l_aiMesh->mName.C_Str();
	l_meshData["VerticesNumber"] = l_aiMesh->mNumVertices;
	auto l_meshFileName = "//Res//ConvertedAssets//" + exportName + "_" + std::to_string(meshIndex) + ".InnoMesh";
	l_meshData["IndicesNumber"] = processMeshData(l_aiMesh, l_meshFileName);
	l_meshData["MeshFile"] = l_meshFileName;
	l_meshData["MeshShapeType"] = MeshShapeType::Custom;

	// process bones
	if (l_aiMesh->mNumBones)
	{
		auto l_skeletonFileName = "//Res//ConvertedAssets//" + exportName + "_" + std::to_string(meshIndex) + ".InnoSkeleton";
		processAssimpBone(l_aiMesh, l_skeletonFileName);
		l_meshData["SkeletonFile"] = l_skeletonFileName;
	}

	// process material
	if (l_aiMesh->mMaterialIndex)
	{
		auto l_materialFileName = "//Res//ConvertedAssets//" + exportName + "_" + std::to_string(meshIndex) + ".InnoMaterial";
		processAssimpMaterial(scene->mMaterials[l_aiMesh->mMaterialIndex], l_materialFileName);
		l_meshData["MaterialFile"] = l_materialFileName;
	}

	return l_meshData;
}

size_t InnoFileSystemNS::AssimpWrapper::processMeshData(const aiMesh * aiMesh, const std::string& exportFileRelativePath)
{
	auto l_verticesNumber = aiMesh->mNumVertices;

	Array<Vertex> l_vertices;

	l_vertices.reserve(l_verticesNumber);
	l_vertices.fulfill();

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
			l_Vertex.m_normal.z = 1.0f;
		}

		l_Vertex.m_pad1 = InnoMath::minVec2<float>;
		l_Vertex.m_pad2 = InnoMath::minVec4<float>;

		l_vertices[i] = l_Vertex;
	}

	Array<Index> l_indices;
	size_t l_indiceSize = 0;

	// bones weight
	if (aiMesh->mNumBones)
	{
		for (size_t i = 0; i < aiMesh->mNumBones; i++)
		{
			auto l_bone = aiMesh->mBones[i];
			if (l_bone->mNumWeights)
			{
				for (unsigned int j = 0; j < l_bone->mNumWeights; j++)
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

		for (unsigned int i = 0; i < l_verticesNumber; i++)
		{
			l_indices[i] = i;
		}

		l_indiceSize = l_verticesNumber;
	}
	else
	{
		if (aiMesh->mNumFaces)
		{
			// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
			for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
			{
				aiFace l_face = aiMesh->mFaces[i];
				l_indiceSize += l_face.mNumIndices;
			}

			l_indices.reserve(l_indiceSize);
			l_indices.fulfill();

			unsigned int l_index = 0;
			for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
			{
				aiFace l_face = aiMesh->mFaces[i];
				// retrieve all indices of the face and store them in the indices vector
				for (unsigned int j = 0; j < l_face.mNumIndices; j++)
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

			for (unsigned int i = 0; i < l_verticesNumber; i++)
			{
				l_indices[i] = i;
			}

			l_indiceSize = l_verticesNumber;
		}
	}

	std::ofstream l_file(IOService::getWorkingDirectory() + exportFileRelativePath, std::ios::binary);

	IOService::serializeVector(l_file, l_vertices);
	IOService::serializeVector(l_file, l_indices);

	l_file.close();

	return l_indiceSize;
}

void InnoFileSystemNS::AssimpWrapper::processAssimpBone(const aiMesh * aiMesh, const std::string& exportFileRelativePath)
{
	json j;
	for (unsigned int i = 0; i < aiMesh->mNumBones; i++)
	{
		json j_child;

		auto l_bone = aiMesh->mBones[i];

		j_child["BoneID"] = i;

		aiQuaternion l_aiRot;
		aiVector3D l_aiPos;
		l_bone->mOffsetMatrix.DecomposeNoScaling(l_aiRot, l_aiPos);
		auto l_rot = Vec4(l_aiRot.x, l_aiRot.y, l_aiRot.z, l_aiRot.w);
		auto l_pos = Vec4(l_aiPos.x, l_aiPos.y, l_aiPos.z, 1.0f);

		JSONParser::to_json(j_child["OffsetRotation"], l_rot);
		JSONParser::to_json(j_child["OffsetPosition"], l_pos);

		j["Bones"].emplace_back(j_child);
	}

	JSONParser::saveJsonDataToDisk(exportFileRelativePath, j);
}

/*
aiTextureType::aiTextureType_NORMALS TextureUsageType::NORMAL map_Kn normal map texture
aiTextureType::aiTextureType_DIFFUSE TextureUsageType::ALBEDO map_Kd albedo texture
aiTextureType::aiTextureType_SPECULAR TextureUsageType::METALLIC map_Ks metallic texture
aiTextureType::aiTextureType_AMBIENT TextureUsageType::ROUGHNESS map_Ka roughness texture
aiTextureType::aiTextureType_EMISSIVE TextureUsageType::AMBIENT_OCCLUSION map_emissive AO texture
aiTextureType::AI_MATKEY_COLOR_DIFFUSE Kd Albedo RGB
aiTextureType::AI_MATKEY_COLOR_TRANSPARENT Tf Alpha A
aiTextureType::AI_MATKEY_COLOR_SPECULAR Ks Metallic
aiTextureType::AI_MATKEY_COLOR_AMBIENT Ka Roughness
aiTextureType::AI_MATKEY_COLOR_EMISSIVE Ke AO
aiTextureType::AI_MATKEY_COLOR_REFLECTIVE Thickness
*/

void InnoFileSystemNS::AssimpWrapper::processAssimpMaterial(const aiMaterial * aiMaterial, const std::string& exportFileRelativePath)
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
				InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", l_AssString.C_Str(), " is unknown texture type!");
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::Sampler2D, TextureUsageType::Normal));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::Sampler2D, TextureUsageType::Albedo));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::Sampler2D, TextureUsageType::Metallic));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::Sampler2D, TextureUsageType::Roughness));
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				l_materialData["Textures"].emplace_back(processTextureData(l_localPath, TextureSamplerType::Sampler2D, TextureUsageType::AmbientOcclusion));
			}
			else
			{
				InnoLogger::Log(LogLevel::Warning, "FileSystem: AssimpWrapper: ", l_AssString.C_Str(), " is unsupported texture type!");
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

	JSONParser::saveJsonDataToDisk(exportFileRelativePath, l_materialData);
}

json InnoFileSystemNS::AssimpWrapper::processTextureData(const std::string & fileName, TextureSamplerType samplerType, TextureUsageType usageType)
{
	json j;

	j["SamplerType"] = samplerType;
	j["UsageType"] = usageType;
	j["File"] = fileName;

	return j;
}

/*
Binary data type:
Duration:double
NumChannels:unsigned int
ChannelIndex:unsigned int
NumKeys:unsigned int
Key:Key(Vec4+Vec4+double)

Binary data structure:
|Duration
|NumChannels
|ChannelIndex1
	|NumKeys
		|Key1
		|Key2
		|...
		|KeyN
|ChannelIndex2
|...
|ChannelIndexN
*/

void InnoFileSystemNS::AssimpWrapper::processAssimpAnimation(const aiAnimation * aiAnimation, const std::string& exportFileRelativePath)
{
	//std::ofstream l_file(IOService::getWorkingDirectory() + exportFileRelativePath, std::ios::binary);

	auto l_duration = aiAnimation->mDuration;
	//IOService::serialize(l_file, &l_duration, sizeof(decltype(l_duration)));

	auto l_numChannels = aiAnimation->mNumChannels;
	if (l_numChannels)
	{
		auto l_numKeys = aiAnimation->mChannels[0]->mNumPositionKeys;

		std::vector<Vec4> l_textureData;

		// Position-xyz, time-w, rotation-xyzw
		l_textureData.reserve(l_numChannels * l_numKeys * 2);

		//IOService::serialize(l_file, &l_numChannels, sizeof(decltype(l_numChannels)));
		for (unsigned int i = 0; i < l_numChannels; i++)
		{
			auto l_channel = aiAnimation->mChannels[i];
			//auto l_channelIndex = i;
			//IOService::serialize(l_file, &l_channelIndex, sizeof(decltype(l_channelIndex)));

			if (l_channel->mNumPositionKeys != l_channel->mNumRotationKeys)
			{
				InnoLogger::Log(LogLevel::Error, "FileSystem: AssimpWrapper: Position key number is different than rotation key number in node: ", l_channel->mNodeName.C_Str(), "!");
				//l_file.close();
				return;
			}

			//IOService::serialize(l_file, &l_channel->mNumPositionKeys, sizeof(decltype(l_channel->mNumPositionKeys)));
			for (unsigned int j = 0; j < l_channel->mNumPositionKeys; j++)
			{
				auto l_posKey = l_channel->mPositionKeys[j];
				auto l_posKeyTime = l_posKey.mTime;
				auto l_posKeyValue = l_posKey.mValue;
				auto l_pos = Vec4(l_posKeyValue.x, l_posKeyValue.y, l_posKeyValue.z, (float)l_posKeyTime);

				l_textureData.emplace_back(l_pos);

				auto l_rotKey = l_channel->mRotationKeys[j];
				auto l_rotKeyValue = l_rotKey.mValue;
				auto l_rot = Vec4(l_rotKeyValue.x, l_rotKeyValue.y, l_rotKeyValue.z, l_rotKeyValue.w);

				l_textureData.emplace_back(l_rot);

				//IOService::serialize(l_file, &l_key, sizeof(decltype(l_key)));
			}
		}

		stbi_write_png((IOService::getWorkingDirectory() + exportFileRelativePath).c_str(), (int)l_numKeys * 2 * 4, (int)l_numChannels, 4, l_textureData.data(), (int)l_numKeys * 2 * sizeof(Vec4));
	}

	//l_file.close();
}