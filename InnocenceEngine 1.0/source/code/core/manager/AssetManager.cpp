#include "../../main/stdafx.h"
#include "AssetManager.h"

AssetManager::AssetManager()
{
}

AssetManager::~AssetManager()
{
}

void AssetManager::init()
{
}

void AssetManager::update()
{
}

void AssetManager::shutdown()
{
}

void AssetManager::loadMesh(const std::string& filePath, const std::vector<StaticMeshData>& meshData)
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene = l_assImporter.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!l_assScene || l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		LogManager::getInstance().printLog("ERROR::ASSIMP:: " + std::string{ l_assImporter.GetErrorString() });
	}
	// retrieve the directory path of the filepath
	//directory = filePath.substr(0, filePath.find_last_of('/'));

	// process ASSIMP's root node recursively

	// process each mesh located at the current node
	for (unsigned int i = 0; i < l_assScene->mRootNode->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* l_assMesh = l_assScene->mMeshes[l_assScene->mRootNode->mMeshes[i]];
		StaticMeshData l_staticMeshData;
		//TODO: finish it
		meshData.push_back(l_staticMeshData);
	}
}

void AssetManager::processAssimpMesh(aiMesh *mesh, const aiScene * scene, const StaticMeshData* meshData)
{
	// Walk through each of the mesh's vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		VertexData vertex;
		// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
						 // positions
		vertex.m_pos.x = mesh->mVertices[i].x;
		vertex.m_pos.y = mesh->mVertices[i].y;
		vertex.m_pos.z = mesh->mVertices[i].z;

		// normals
		vertex.m_normal.x = mesh->mNormals[i].x;
		vertex.m_normal.y = mesh->mNormals[i].y;
		vertex.m_normal.z = mesh->mNormals[i].z;

		// texture coordinates
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			glm::vec2 vec;
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vertex.m_texCoord.x = mesh->mTextureCoords[0][i].x;
			vertex.m_texCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
		vertex.m_texCoord.x = 0.0f;
		vertex.m_texCoord.y = 0.0f;
		}

		//// tangent
		//vector.x = mesh->mTangents[i].x;
		//vector.y = mesh->mTangents[i].y;
		//vector.z = mesh->mTangents[i].z;
		//vertex.Tangent = vector;

		//// bitangent
		//vector.x = mesh->mBitangents[i].x;
		//vector.y = mesh->mBitangents[i].y;
		//vector.z = mesh->mBitangents[i].z;
		//vertex.Bitangent = vector;

		//vertices.push_back(vertex);
	}
	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			meshData->addIndex(face.mIndices[j]);
	}
}
