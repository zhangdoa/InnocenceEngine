
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
		LogManager::getInstance().printLog("ERROR:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}
	if (l_assImporter.ReadFile("../res/models/" + fileName.substr(0, fileName.find(".")) + ".innoModel", aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) != nullptr)
	{
		LogManager::getInstance().printLog("Model: " + fileName + " has been already imported.");
		return;
	}
	Assimp::Exporter l_assExporter;
	l_assExporter.Export(l_assScene, "assbin", "../res/models/" + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
	LogManager::getInstance().printLog("Model: " + fileName + " is imported.");
}

void AssetManager::loadModel(const std::string & fileName, VisibleComponent & visibleComponent) const
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	const aiScene* l_assScene = l_assImporter.ReadFile("../res/models/" + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	// check for errors
	if (!l_assScene || l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		LogManager::getInstance().printLog("ERROR:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}
	std::string l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);
	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (l_assScene->mRootNode->mNumMeshes > 0)
	{
		processAssimpNode(l_fileName, l_assScene->mRootNode, l_assScene, visibleComponent);
		visibleComponent.getGraphicData()[0].setVisiblilityType(visibleComponent.getVisiblilityType());
		visibleComponent.getGraphicData()[0].init();
	}
	for (unsigned int i = 0; i < l_assScene->mRootNode->mNumChildren; i++)
	{
		if (l_assScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			processAssimpNode(l_fileName, l_assScene->mRootNode->mChildren[i], l_assScene, visibleComponent);
		}
	}
	// initialize mesh datas
	for (unsigned int i = 0; i < visibleComponent.getGraphicData().size(); i++)
	{
		visibleComponent.getGraphicData()[i].setVisiblilityType(visibleComponent.getVisiblilityType());
		visibleComponent.getGraphicData()[i].init();
	}
	LogManager::getInstance().printLog("innoModel: " + fileName +  " is loaded.");
}

void AssetManager::processAssimpNode(const std::string& fileName, aiNode * node, const aiScene * scene, VisibleComponent & visibleComponent) const
{
	visibleComponent.addGraphicData();
	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		processAssimpMesh(scene->mMeshes[node->mMeshes[i]], visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getMeshData());
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			processAssimpMaterial(fileName, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex], visibleComponent);
		}
	}
}

void AssetManager::processAssimpMesh(aiMesh*mesh, MeshData& meshData) const
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

		// tangent
		if (mesh->mTangents)
		{
			vertexData.m_tangent.x = mesh->mTangents[i].x;
			vertexData.m_tangent.y = mesh->mTangents[i].y;
			vertexData.m_tangent.z = mesh->mTangents[i].z;
		}
		else
		{
			vertexData.m_tangent.x = 0.0f;
			vertexData.m_tangent.y = 0.0f;
			vertexData.m_tangent.z = 0.0f;
		}


		// bitangent
		if (mesh->mBitangents)
		{
			vertexData.m_bitangent.x = mesh->mBitangents[i].x;
			vertexData.m_bitangent.y = mesh->mBitangents[i].y;
			vertexData.m_bitangent.z = mesh->mBitangents[i].z;
		}
		else
		{
			vertexData.m_bitangent.x = 0.0f;
			vertexData.m_bitangent.y = 0.0f;
			vertexData.m_bitangent.z = 0.0f;
		}

		meshData.getVertices().emplace_back(vertexData);
	}

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			meshData.getIntices().emplace_back(face.mIndices[j]);
		}
	}
	LogManager::getInstance().printLog("innoMesh is loaded.");
}

void AssetManager::processAssimpMaterial(const std::string& fileName, aiMaterial* material, VisibleComponent & visibleComponent) const
{
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		TextureData newTextureData;
		visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().emplace_back(newTextureData);
		loadTexture(fileName, material, aiTextureType_DIFFUSE, visibleComponent);
	}
	if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
	{
		TextureData newTextureData;
		visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().emplace_back(newTextureData);
		loadTexture(fileName, material, aiTextureType_SPECULAR, visibleComponent);
	}
	if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
	{
		TextureData newTextureData;
		visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().emplace_back(newTextureData);
		loadTexture(fileName, material, aiTextureType_NORMALS, visibleComponent);
	}
}

void AssetManager::loadTexture(const std::string& fileName, aiMaterial* material, aiTextureType aiTextureType, VisibleComponent & visibleComponent) const
{
	aiString l_AssString;
	for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType); i++)
	{
		material->GetTexture(aiTextureType, i, &l_AssString);
		// set local path, remove slash
		std::string l_localPath;
		if (std::string(l_AssString.C_Str()).find_last_of('//') != std::string::npos)
		{
			l_localPath = std::string(l_AssString.C_Str()).substr(std::string(l_AssString.C_Str()).find_last_of("//"));
		}
		else if (std::string(l_AssString.C_Str()).find_last_of('\\') != std::string::npos)
		{
			l_localPath = std::string(l_AssString.C_Str()).substr(std::string(l_AssString.C_Str()).find_last_of("\\"));
		}
		else
		{
			l_localPath = std::string(l_AssString.C_Str());
		}

		visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData()[visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().size() -1].setTextureType(textureType(aiTextureType));
		visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData()[visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().size() - 1].setTextureWrapMethod(visibleComponent.getTextureWrapMethod());
		// load image
		int width, height, nrChannels;

		stbi_set_flip_vertically_on_load(true);

		auto *data = stbi_load(("../res/textures/" + fileName + "//" + l_localPath).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData()[visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().size() - 1].init();
			visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData()[visibleComponent.getGraphicData()[visibleComponent.getGraphicData().size() - 1].getTextureData().size() - 1].sendDataToGPU(aiTextureType - 1, nrChannels, width, height, data);
			LogManager::getInstance().printLog("innoTexture: " + l_localPath + " is loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR:STBI: Failed to load texture: " + l_localPath);
		}
		stbi_image_free(data);
	}
}

void AssetManager::loadTexture(const std::string & fileName, VisibleComponent & visibleComponent) const
{
	TextureData newTextureData;
	visibleComponent.getGraphicData()[0].getTextureData().emplace_back(newTextureData);
	visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].setTextureType(textureType::DIFFUSE);
	visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].setTextureWrapMethod(visibleComponent.getGraphicData()[0].getTextureWrapMethod());
	visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].init();
	int width, height, nrChannels;
	// load image
	stbi_set_flip_vertically_on_load(true);
	auto *data = stbi_load(("../res/textures/" + fileName).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].sendDataToGPU(0, nrChannels, width, height, data);
		LogManager::getInstance().printLog("innoTexture: " + fileName + " is loaded.");
	}
	else
	{
		LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + fileName);
	}
	stbi_image_free(data);
}

void AssetManager::loadTexture(const std::vector<std::string>& fileName, VisibleComponent & visibleComponent) const
{
	TextureData newTextureData;
	visibleComponent.getGraphicData()[0].getTextureData().emplace_back(newTextureData);
	visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].setTextureType(textureType::CUBEMAP);
	visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].setTextureWrapMethod(visibleComponent.getGraphicData()[0].getTextureWrapMethod());
	visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].init();
	int width, height, nrChannels;
	for (unsigned int i = 0; i < fileName.size(); i++)
	{
		// load image, do not flip texture
		stbi_set_flip_vertically_on_load(false);
		auto *data = stbi_load(("../res/textures/" + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			visibleComponent.getGraphicData()[0].getTextureData()[visibleComponent.getGraphicData()[0].getTextureData().size() - 1].sendDataToGPU(i, nrChannels, width, height, data);
			LogManager::getInstance().printLog("innoTexture: " + fileName[i] + " is loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + ("../res/textures/" + fileName[i]));
		}
		stbi_image_free(data);
	}
}
