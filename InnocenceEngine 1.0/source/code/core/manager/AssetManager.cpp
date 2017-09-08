
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

std::string AssetManager::loadShader(const std::string & shaderFileName) const
{
	std::ifstream file;
	file.open(("../res/shaders/" + shaderFileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

void AssetManager::importModel(const std::string& fileName) const
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene = l_assImporter.ReadFile("../res/models/" + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!l_assScene || l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		LogManager::getInstance().printLog("ERROR::ASSIMP:: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}
	Assimp::Exporter l_assExporter;
	l_assExporter.Export(l_assScene, "assbin", "../res/models/" + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
	LogManager::getInstance().printLog("Model imported.");
}

void AssetManager::loadModel(const std::string & fileName, std::vector<StaticMeshData>& staticMeshDatas) const
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene = l_assImporter.ReadFile("../res/models/" + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!l_assScene || l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		LogManager::getInstance().printLog("ERROR::ASSIMP:: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}
	processAssimpNode(l_assScene->mRootNode, l_assScene, staticMeshDatas);
	for (auto i = 0; i < l_assScene->mRootNode->mNumChildren; i++)
	{
		processAssimpNode(l_assScene->mRootNode->mChildren[i], l_assScene, staticMeshDatas);
	}
	LogManager::getInstance().printLog("innoModel loaded.");
}

void AssetManager::processAssimpNode(aiNode * node, const aiScene * scene, std::vector<StaticMeshData>& staticMeshDatas) const
{
	// process each mesh located at the current node
	for (auto i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		StaticMeshData staticMeshData;
		staticMeshData.init();
		processAssimpMesh(scene->mMeshes[node->mMeshes[i]], scene, staticMeshData);
		staticMeshDatas.emplace_back(staticMeshData);
	}
}

void AssetManager::processAssimpMesh(aiMesh *mesh, const aiScene * scene, StaticMeshData& staticMeshData) const
{
	for (auto i = 0; i < mesh->mNumVertices; i++)
	{
		VertexData vertexData;
		// positions
		vertexData.m_pos.x = mesh->mVertices[i].x;
		vertexData.m_pos.y = mesh->mVertices[i].y;
		vertexData.m_pos.z = mesh->mVertices[i].z;

		// texture coordinates
		if (mesh->mTextureCoords[0])
		{
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vertexData.m_texCoord.x = mesh->mTextureCoords[0][i].x;
			vertexData.m_texCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			vertexData.m_texCoord.x = 0.0f;
			vertexData.m_texCoord.y = 0.0f;
		}

		// normals
		if (mesh->mNormals)
		{
			vertexData.m_normal.x = mesh->mNormals[i].x;
			vertexData.m_normal.y = mesh->mNormals[i].y;
			vertexData.m_normal.z = mesh->mNormals[i].z;
		}
		else
		{
			vertexData.m_normal.x = 0.0f;
			vertexData.m_normal.y = 0.0f;
			vertexData.m_normal.z = 0.0f;
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

		staticMeshData.getVertices().emplace_back(vertexData);
	}
	
	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (auto i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = 0; j < face.mNumIndices; j++)
		{
			staticMeshData.getIntices().emplace_back(face.mIndices[j]);
		}
	}

	LogManager::getInstance().printLog("innoMesh loaded.");
}


