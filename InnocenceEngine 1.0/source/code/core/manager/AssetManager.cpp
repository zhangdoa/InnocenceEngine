
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
	m_UnitCubeTemplate = RenderingManager::getInstance().addMeshData();
	auto lastMeshData = &RenderingManager::getInstance().getMeshData(m_UnitCubeTemplate);
	lastMeshData->addUnitCube();
	lastMeshData->initialize();
	lastMeshData->sendDataToGPU(false, false);

	m_UnitSphereTemplate = RenderingManager::getInstance().addMeshData();
	lastMeshData = &RenderingManager::getInstance().getMeshData(m_UnitSphereTemplate);
	lastMeshData->addUnitSphere();
	lastMeshData->setMeshDrawMethod(meshDrawMethod::TRIANGLE_STRIP);
	lastMeshData->initialize();
	lastMeshData->sendDataToGPU(false, false);

	m_UnitQuadTemplate = RenderingManager::getInstance().addMeshData();
	lastMeshData = &RenderingManager::getInstance().getMeshData(m_UnitQuadTemplate);
	lastMeshData->addUnitQuad();
	lastMeshData->initialize();
	lastMeshData->sendDataToGPU(true, true);

	m_basicNormalTemplate = loadTextureFromDisk("basic_normal.png", textureType::NORMALS, textureWrapMethod::REPEAT);
	m_basicAlbedoTemplate = loadTextureFromDisk("basic_diffuse.png", textureType::DIFFUSE, textureWrapMethod::REPEAT);
	m_basicMetallicTemplate = loadTextureFromDisk("basic_specular.png", textureType::SPECULAR, textureWrapMethod::REPEAT);
	m_basicRoughnessTemplate = loadTextureFromDisk("basic_roughness.png", textureType::AMBIENT, textureWrapMethod::REPEAT);
	m_basicAOTemplate = loadTextureFromDisk("basic_ao.png", textureType::EMISSIVE, textureWrapMethod::REPEAT);
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

void AssetManager::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	loadModelImpl(fileName, visibleComponent);
	assignDefaultTextures(visibleComponent);
}

void AssetManager::loadModelImpl(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_loadedAssetData = m_loadedModelMap.find(fileName);
	// check if this file has already loaded
	if (l_loadedAssetData != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedAssetData->second, visibleComponent);
		LogManager::getInstance().printLog("innoModel: " + fileName + " is already loaded, successfully assigned loaded graphic data IDs.");
	}
	else
	{
		loadModelFromDisk(fileName, visibleComponent);
	}
}

void AssetManager::assignloadedModel(graphicDataMap& loadedGraphicDataMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setGraphicDataMap(loadedGraphicDataMap);
}

void AssetManager::loadModelFromDisk(const std::string& fileName, VisibleComponent& visibleComponent)
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
	// record that file's been loaded 
	m_loadedModelMap.emplace(fileName, graphicDataMap());

	// find corresponded graphicDataMap
	auto l_graphicDataMap = &m_loadedModelMap.find(fileName)->second;

	// only need last part of file name without subfix as material's subfolder name
	std::string l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);

	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (l_assScene->mRootNode->mNumMeshes > 0)
	{
		processAssimpNode(l_fileName, l_assScene->mRootNode, l_assScene, l_graphicDataMap, visibleComponent);
	}
	for (auto i = (unsigned int)0; i < l_assScene->mRootNode->mNumChildren; i++)
	{
		if (l_assScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			processAssimpNode(l_fileName, l_assScene->mRootNode->mChildren[i], l_assScene, l_graphicDataMap, visibleComponent);
		}
	}

	visibleComponent.setGraphicDataMap(*l_graphicDataMap);

	LogManager::getInstance().printLog("innoModel: " + fileName + " is loaded.");
}


void AssetManager::processAssimpNode(const std::string& fileName, aiNode * node, const aiScene * scene, graphicDataMap* graphicDataMap, VisibleComponent & visibleComponent)
{
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_meshDataID = RenderingManager::getInstance().addMeshData();

		graphicDataMap->emplace(l_meshDataID, textureDataMap());

		processSingleAssimpMesh(scene->mMeshes[node->mMeshes[i]], l_meshDataID, visibleComponent.getMeshDrawMethod());

		// process material if there was anyone
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			auto l_textureDataMap = &graphicDataMap->find(l_meshDataID)->second;

			processSingleAssimpMaterial(fileName, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex], visibleComponent.getTextureWrapMethod(), l_textureDataMap);
		}
	}
}

void AssetManager::processSingleAssimpMesh(aiMesh*mesh, meshDataID meshDataID, meshDrawMethod meshDrawMethod) const
{
	auto lastMeshData = &RenderingManager::getInstance().getMeshData(meshDataID);

	for (auto i = (unsigned int)0; i < mesh->mNumVertices; i++)
	{
		addVertexData(mesh, i, lastMeshData);
	}

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (auto i = (unsigned int)0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = (unsigned int)0; j < face.mNumIndices; j++)
		{
			lastMeshData->getIntices().emplace_back(face.mIndices[j]);
		}
	}
	lastMeshData->setMeshDrawMethod(meshDrawMethod);
	lastMeshData->initialize();
	lastMeshData->sendDataToGPU(false, false);
	LogManager::getInstance().printLog("innoMesh is loaded.");
}

void AssetManager::addVertexData(aiMesh * aiMesh, int vertexIndex, MeshData * meshData) const
{
	GLVertexData vertexData;
	// positions
	vertexData.m_pos.x = aiMesh->mVertices[vertexIndex].x;
	vertexData.m_pos.y = aiMesh->mVertices[vertexIndex].y;
	vertexData.m_pos.z = aiMesh->mVertices[vertexIndex].z;

	// texture coordinates
	if (aiMesh->mTextureCoords[0])
	{
		// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
		// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
		vertexData.m_texCoord.x = aiMesh->mTextureCoords[0][vertexIndex].x;
		vertexData.m_texCoord.y = aiMesh->mTextureCoords[0][vertexIndex].y;
	}
	else
	{
		vertexData.m_texCoord.x = 0.0f;
		vertexData.m_texCoord.y = 0.0f;
	}

	// normals
	if (aiMesh->mNormals)
	{
		vertexData.m_normal.x = aiMesh->mNormals[vertexIndex].x;
		vertexData.m_normal.y = aiMesh->mNormals[vertexIndex].y;
		vertexData.m_normal.z = aiMesh->mNormals[vertexIndex].z;
	}
	else
	{
		vertexData.m_normal.x = 0.0f;
		vertexData.m_normal.y = 0.0f;
		vertexData.m_normal.z = 0.0f;
	}

	// tangent
	if (aiMesh->mTangents)
	{
		vertexData.m_tangent.x = aiMesh->mTangents[vertexIndex].x;
		vertexData.m_tangent.y = aiMesh->mTangents[vertexIndex].y;
		vertexData.m_tangent.z = aiMesh->mTangents[vertexIndex].z;
	}
	else
	{
		vertexData.m_tangent.x = 0.0f;
		vertexData.m_tangent.y = 0.0f;
		vertexData.m_tangent.z = 0.0f;
	}


	// bitangent
	if (aiMesh->mBitangents)
	{
		vertexData.m_bitangent.x = aiMesh->mBitangents[vertexIndex].x;
		vertexData.m_bitangent.y = aiMesh->mBitangents[vertexIndex].y;
		vertexData.m_bitangent.z = aiMesh->mBitangents[vertexIndex].z;
	}
	else
	{
		vertexData.m_bitangent.x = 0.0f;
		vertexData.m_bitangent.y = 0.0f;
		vertexData.m_bitangent.z = 0.0f;
	}

	meshData->getVertices().emplace_back(vertexData);
}

void AssetManager::processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod, textureDataMap* textureDataMap) const
{
	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			loadTextureForModel(fileName, aiMaterial, aiTextureType(i), textureWrapMethod, textureDataMap);
		}
	}
}

void AssetManager::loadTextureForModel(const std::string& fileName, aiMaterial * aiMaterial, aiTextureType aiTextureType, textureWrapMethod textureWrapMethod, textureDataMap* textureDataMap) const
{
	aiString l_AssString;
	for (auto i = (unsigned int)0; i < aiMaterial->GetTextureCount(aiTextureType); i++)
	{
		aiMaterial->GetTexture(aiTextureType, i, &l_AssString);
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

		// load image
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(true);
		auto data = stbi_load(("../res/textures/" + fileName + "//" + l_localPath).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			auto l_textureDataID = RenderingManager::getInstance().addTextureData();
			textureDataMap->emplace(textureType(aiTextureType), l_textureDataID);
			auto lastTextureData = &RenderingManager::getInstance().getTextureData(l_textureDataID);
			lastTextureData->setTextureType(textureType(aiTextureType));
			lastTextureData->setTextureWrapMethod(textureWrapMethod);
			lastTextureData->initialize();
			lastTextureData->sendDataToGPU(aiTextureType - 1, nrChannels, width, height, data);
			LogManager::getInstance().printLog("innoTexture: " + l_localPath + " is loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR:STBI: Failed to load texture: " + l_localPath);
		}
		//stbi_image_free(data);
	}
}

void AssetManager::loadSingleTexture(const std::string & fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	auto l_loadedTextureData = m_loadedTextureMap.find(fileName);
	// check if this file has already loaded
	if (l_loadedTextureData != m_loadedTextureMap.end())
	{
		assignloadedTexture(l_loadedTextureData->second, visibleComponent);
		LogManager::getInstance().printLog("innoTexture: " + fileName + " is already loaded, successfully assigned loaded texture data IDs.");
	}
	else
	{	
		auto l_textureDataPair = textureDataPair(textureType, loadTextureFromDisk(fileName, textureType, visibleComponent.getTextureWrapMethod()));
		m_loadedTextureMap.emplace(fileName, l_textureDataPair);
		assignloadedTexture(l_textureDataPair, visibleComponent);
	}
}

void AssetManager::assignloadedTexture(textureDataPair& loadedTextureDataPair, VisibleComponent & visibleComponent)
{
	for (auto& l_graphicData : visibleComponent.getGraphicDataMap())
	{
		auto& l_texturePair = l_graphicData.second.find(loadedTextureDataPair.first);
		if (l_texturePair != l_graphicData.second.end())
		{
			l_texturePair->second = loadedTextureDataPair.second;
		}
		else
		{
			l_graphicData.second.emplace(loadedTextureDataPair.first, loadedTextureDataPair.second);
		}
	}
}

textureDataID AssetManager::loadTextureFromDisk(const std::string & fileName, textureType textureType, textureWrapMethod textureWrapMethod)
{
	int width, height, nrChannels;
	// load image
	stbi_set_flip_vertically_on_load(true);
	auto *data = stbi_load(("../res/textures/" + fileName).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		auto id = RenderingManager::getInstance().addTextureData();
		auto lastTextureData = &RenderingManager::getInstance().getTextureData(id);
		lastTextureData->setTextureType(textureType);
		lastTextureData->setTextureWrapMethod(textureWrapMethod);
		lastTextureData->initialize();
		lastTextureData->sendDataToGPU(0, nrChannels, width, height, data);
		LogManager::getInstance().printLog("innoTexture: " + fileName + " is loaded.");
		return id;
	}
	else
	{
		LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + fileName);
		return 0;
	}
	//stbi_image_free(data);
}

void AssetManager::loadCubeMapTextures(const std::vector<std::string>& fileName, VisibleComponent & visibleComponent) const
{
	int width, height, nrChannels;
	for (auto i = (unsigned int)0; i < fileName.size(); i++)
	{
		// load image, do not flip texture
		stbi_set_flip_vertically_on_load(false);
		auto *data = stbi_load(("../res/textures/" + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			auto id = RenderingManager::getInstance().addTextureData();
			auto lastTextureData = &RenderingManager::getInstance().getTextureData(id);
			lastTextureData->setTextureType(textureType::CUBEMAP);
			lastTextureData->setTextureWrapMethod(visibleComponent.getTextureWrapMethod());
			lastTextureData->initialize();
			lastTextureData->sendDataToGPU(i, nrChannels, width, height, data);
			visibleComponent.addTextureData(id, textureType::CUBEMAP);
			LogManager::getInstance().printLog("innoTexture: " + fileName[i] + " is loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + ("../res/textures/" + fileName[i]));
		}
		//stbi_image_free(data);
	}
}


void AssetManager::assignDefaultTextures(VisibleComponent & visibleComponent) const
{
	for (auto& l_graphicData : visibleComponent.getGraphicDataMap())
	{
		auto l_textureDataMap = &l_graphicData.second;

		if (l_textureDataMap->find(textureType::NORMALS) == l_textureDataMap->end())
		{
			l_textureDataMap->emplace(textureType::NORMALS, m_basicNormalTemplate);
		};
		if (l_textureDataMap->find(textureType::DIFFUSE) == l_textureDataMap->end())
		{
			l_textureDataMap->emplace(textureType::DIFFUSE, m_basicAlbedoTemplate);
		};
		if (l_textureDataMap->find(textureType::SPECULAR) == l_textureDataMap->end())
		{
			l_textureDataMap->emplace(textureType::SPECULAR, m_basicMetallicTemplate);
		};
		if (l_textureDataMap->find(textureType::AMBIENT) == l_textureDataMap->end())
		{
			l_textureDataMap->emplace(textureType::AMBIENT, m_basicRoughnessTemplate);
		};
		if (l_textureDataMap->find(textureType::EMISSIVE) == l_textureDataMap->end())
		{
			l_textureDataMap->emplace(textureType::EMISSIVE, m_basicAOTemplate);
		};
	}
}

void AssetManager::addUnitCube(VisibleComponent & visibleComponent) const
{
	visibleComponent.addMeshData(m_UnitCubeTemplate);
	if (visibleComponent.getVisiblilityType() == visiblilityType::STATIC_MESH)
	{
		assignDefaultTextures(visibleComponent);
	}
}


void AssetManager::addUnitSphere(VisibleComponent & visibleComponent) const
{
	visibleComponent.addMeshData(m_UnitSphereTemplate);
	if (visibleComponent.getVisiblilityType() == visiblilityType::STATIC_MESH)
	{
		assignDefaultTextures(visibleComponent);
	}
}

void AssetManager::addUnitQuad(VisibleComponent & visibleComponent) const
{
	visibleComponent.addMeshData(m_UnitQuadTemplate);
}
