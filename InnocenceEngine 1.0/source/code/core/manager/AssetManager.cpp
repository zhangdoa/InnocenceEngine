
#include "../../main/stdafx.h"
#include "AssetManager.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "../third-party/stb_image.h"

AssetManager::AssetManager()
{
}

AssetManager::~AssetManager()
{
}

void AssetManager::initialize()
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

void AssetManager::loadModel(const std::string & fileName, VisibleComponent & visibleComponent) const
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
	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (l_assScene->mRootNode->mNumMeshes > 0)
	{
		visibleComponent.addMeshData();
		processAssimpNode(l_assScene->mRootNode, l_assScene, visibleComponent.getMeshData()[0]);
		visibleComponent.getMeshData()[0].init();
		visibleComponent.getMeshData()[0].sendDataToGPU();
	}
	
	// @TODO: deal with the node
	for (unsigned int i = 0; i < l_assScene->mRootNode->mNumChildren; i++)
	{
		if (l_assScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			visibleComponent.addMeshData();
			processAssimpNode(l_assScene->mRootNode->mChildren[i], l_assScene, visibleComponent.getMeshData()[visibleComponent.getMeshData().size() - 1]);
		}
	}
	// initialize mesh datas
	for (unsigned int i = 0; i < visibleComponent.getMeshData().size(); i++)
	{
		visibleComponent.getMeshData()[i].init();
		visibleComponent.getMeshData()[i].sendDataToGPU();
	}
	LogManager::getInstance().printLog("innoModel loaded.");
}

void AssetManager::processAssimpNode(aiNode * node, const aiScene * scene, MeshData& staticMeshData) const
{
	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		processAssimpMesh(scene->mMeshes[node->mMeshes[i]], staticMeshData);
	}
}

void AssetManager::processAssimpMesh(aiMesh *mesh, MeshData& staticMeshData) const
{
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		GLVertexData vertexData;
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
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			staticMeshData.getIntices().emplace_back(face.mIndices[j]);
		}
	}
	LogManager::getInstance().printLog("innoMesh loaded.");
}


void AssetManager::loadTexture(const std::string & fileName, VisibleComponent & visibleComponent) const
{
	visibleComponent.addTextureData();
	visibleComponent.getTextureData()[0].setTextureType(textureType::ALBEGO);
	visibleComponent.getTextureData()[0].init();
	int width, height, nrChannel;
	// load image, create texture and generate mipmaps
	stbi_set_flip_vertically_on_load(true);
	auto *data = stbi_load(("../res/textures/" + fileName).c_str(), &width, &height, &nrChannel, 0);
	if (data)
	{
		visibleComponent.getTextureData()[0].sendDataToGPU(0, width, height, data);
		LogManager::getInstance().printLog("innoTexture loaded.");
	}
	else
	{
		LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + fileName);
	}
	stbi_image_free(data);
}

void AssetManager::loadTexture(const std::vector<std::string>& fileName, VisibleComponent & visibleComponent) const
{
	visibleComponent.addTextureData();
	visibleComponent.getTextureData()[0].setTextureType(textureType::CUBEMAP);
	visibleComponent.getTextureData()[0].init();
	int width, height, nrChannel;
	for (unsigned int i = 0; i < fileName.size(); i++)
	{
		// load image, create texture and generate mipmaps
		auto *data  = stbi_load(("../res/textures/" + fileName[i]).c_str(), &width, &height, &nrChannel, 0);
		if (data)
		{
			visibleComponent.getTextureData()[0].sendDataToGPU(i, width, height, data);
			LogManager::getInstance().printLog("innoTexture loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + ("../res/textures/" + fileName[i]));
		}
		stbi_image_free(data);
	}
}
