#include "AssimpWrapper.h"

#include "assimp/Importer.hpp"
#include "assimp/Exporter.hpp"
#include "assimp/DefaultLogger.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "../ICoreSystem.h"
extern ICoreSystem* g_pCoreSystem;

#include "FileSystemHelper.h"
#include "JSONParser.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	INNO_PRIVATE_SCOPE AssimpWrapper
	{
		json processAssimpScene(const aiScene* aiScene, const std::string& exportName);
		json processAssimpNode(const aiNode * node, const aiScene * scene, const std::string& exportName);
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
	auto l_exportFileName = getFileName(fileName);
	auto l_exportFileRelativePath = exportPath + l_exportFileName + ".InnoModel";

	// Check if the file was converted already
	if (isFileExist(l_exportFileRelativePath))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: AssimpWrapper: " + fileName + " has already been converted!");
		return true;
	}

	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;

	// Check if the file was exist
	if (isFileExist(fileName))
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: AssimpWrapper: converting " + fileName + "...");
#if defined _DEBUG
		std::string l_logFilePath = getWorkingDirectory() + "res/log/AssimpLog" + fileName + ".txt";
		Assimp::DefaultLogger::create(l_logFilePath.c_str(), Assimp::Logger::VERBOSE);
#endif
		l_assScene = l_assImporter.ReadFile(getWorkingDirectory() + fileName, aiProcess_Triangulate | aiProcess_FlipUVs);
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssimpWrapper: " + fileName + " doesn't exist!");
		return false;
	}
	if (l_assScene)
	{
		if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssimpWrapper: " + std::string{ l_assImporter.GetErrorString() });
			return false;
		}
		auto l_result = processAssimpScene(l_assScene, l_exportFileName);
		JSONParser::saveJsonDataToDisk(l_exportFileRelativePath, l_result);

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: AssimpWrapper: " + fileName + " has been converted.");
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssimpWrapper: can't load file " + fileName + "!");
		return false;
	}

	return true;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpScene(const aiScene* aiScene, const std::string& exportName)
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

	if (aiScene->mNumAnimations)
	{
		for (unsigned int i = 0; i < aiScene->mNumAnimations; i++)
		{
			auto l_animationFileName = "//Res//ConvertedAssets//" + exportName + "_" + std::to_string(i) + ".InnoAnimation";
			processAssimpAnimation(aiScene->mAnimations[i], l_animationFileName);
			l_sceneData["AnimationFiles"].emplace_back(l_animationFileName);
		}
	}

	l_sceneData["Nodes"].emplace_back(processAssimpNode(aiScene->mRootNode, aiScene, exportName));

	return l_sceneData;
}

json InnoFileSystemNS::AssimpWrapper::processAssimpNode(const aiNode * node, const aiScene * scene, const std::string& exportName)
{
	json l_nodeData;

	l_nodeData["NodeName"] = *node->mName.C_Str();

	// process each mesh located at the current node
	if (node->mNumMeshes)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			l_nodeData["Meshes"].emplace_back(processAssimpMesh(scene, exportName, node->mMeshes[i]));
		}
	}

	// process children node
	if (node->mNumChildren)
	{
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			l_nodeData["Nodes"].emplace_back(processAssimpNode(node->mChildren[i], scene, exportName));
		}
	}

	return l_nodeData;
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
			l_Vertex.m_normal.z = 1.0f;
		}
		l_vertices.emplace_back(l_Vertex);
	}

	std::vector<Index> l_indices;
	size_t l_indiceSize = 0;

	// bones weight
	if (aiMesh->mNumBones)
	{
		for (size_t i = 0; i < aiMesh->mNumBones; i++)
		{
			auto l_bone = aiMesh->mBones[i];
			if (l_bone->mNumWeights)
			{
				for (unsigned int i = 0; i < l_bone->mNumWeights; i++)
				{
					aiVertexWeight l_vertexWeight = l_bone->mWeights[i];

					// Only the first 3 most weighted bone will be stored
					if (l_vertexWeight.mWeight > l_vertices[l_vertexWeight.mVertexId].m_pad1.y)
					{
						// 1st weighted bone id and weight
						l_vertices[l_vertexWeight.mVertexId].m_pad1.x = (float)i;
						l_vertices[l_vertexWeight.mVertexId].m_pad1.y = l_vertexWeight.mWeight;
					}
					else if (l_vertexWeight.mWeight > l_vertices[l_vertexWeight.mVertexId].m_pad2.y)
					{
						// 2nd weighted bone id and weight
						l_vertices[l_vertexWeight.mVertexId].m_pad2.x = (float)i;
						l_vertices[l_vertexWeight.mVertexId].m_pad2.y = l_vertexWeight.mWeight;
					}
					else if (l_vertexWeight.mWeight > l_vertices[l_vertexWeight.mVertexId].m_pad2.w)
					{
						// 3rd weighted bone id and weight
						l_vertices[l_vertexWeight.mVertexId].m_pad2.z = (float)i;
						l_vertices[l_vertexWeight.mVertexId].m_pad2.w = l_vertexWeight.mWeight;
					}
				}
			}
		}

		l_indices.reserve(l_verticesNumber);

		for (unsigned int i = 0; i < l_verticesNumber; i++)
		{
			l_indices.emplace_back(i);
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

			for (unsigned int i = 0; i < aiMesh->mNumFaces; i++)
			{
				aiFace l_face = aiMesh->mFaces[i];
				// retrieve all indices of the face and store them in the indices vector
				for (unsigned int j = 0; j < l_face.mNumIndices; j++)
				{
					l_indices.emplace_back(l_face.mIndices[j]);
				}
			}
		}
		else
		{
			l_indices.reserve(l_verticesNumber);

			for (unsigned int i = 0; i < l_verticesNumber; i++)
			{
				l_indices.emplace_back(i);
			}

			l_indiceSize = l_verticesNumber;
		}
	}

	std::ofstream l_file(getWorkingDirectory() + exportFileRelativePath, std::ios::binary);

	serializeVector(l_file, l_vertices);
	serializeVector(l_file, l_indices);

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

		j_child["BoneID"] = std::hash<std::string>()(l_bone->mName.C_Str());

		aiQuaternion l_aiRot;
		aiVector3D l_aiPos;

		l_bone->mOffsetMatrix.DecomposeNoScaling(l_aiRot, l_aiPos);
		auto l_rot = vec4(l_aiRot.x, l_aiRot.y, l_aiRot.z, l_aiRot.w);
		auto l_pos = vec4(l_aiPos.x, l_aiPos.y, l_aiPos.z, 1.0f);

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

void InnoFileSystemNS::AssimpWrapper::processAssimpAnimation(const aiAnimation * aiAnimation, const std::string& exportFileRelativePath)
{
	std::ofstream l_file(getWorkingDirectory() + exportFileRelativePath, std::ios::binary);

	auto l_duration = aiAnimation->mDuration;

	serialize(l_file, &l_duration, sizeof(decltype(l_duration)));

	auto l_numChannels = aiAnimation->mNumChannels;

	if (l_numChannels)
	{
		serialize(l_file, &l_numChannels, sizeof(decltype(l_numChannels)));

		for (unsigned int i = 0; i < l_numChannels; i++)
		{
			auto l_channel = aiAnimation->mChannels[i];

			auto l_channelID = std::hash<std::string>()(l_channel->mNodeName.C_Str());
			serialize(l_file, &l_channelID, sizeof(decltype(l_channelID)));

			if (l_channel->mNumPositionKeys != l_channel->mNumRotationKeys)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: AssimpWrapper: Position key number is different than rotation key number in node: " + std::string(l_channel->mNodeName.C_Str()) + "!");
				l_file.close();
				return;
			}

			for (unsigned int j = 0; j < l_channel->mNumPositionKeys; j++)
			{
				auto l_posKey = l_channel->mPositionKeys[i];
				auto l_posKeyTime = l_posKey.mTime;
				auto l_posKeyValue = l_posKey.mValue;
				vec4 l_pos = vec4(l_posKeyValue.x, l_posKeyValue.y, l_posKeyValue.z, 1.0f);

				auto l_rotKey = l_channel->mRotationKeys[i];
				auto l_rotKeyTime = l_rotKey.mTime;
				auto l_rotKeyValue = l_rotKey.mValue;
				vec4 l_rot = vec4(l_rotKeyValue.x, l_rotKeyValue.y, l_rotKeyValue.z, l_rotKeyValue.w);
			}
		}
	}

	l_file.close();
}