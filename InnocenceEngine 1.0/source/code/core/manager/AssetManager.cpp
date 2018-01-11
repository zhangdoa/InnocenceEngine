
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

void AssetManager::setup()
{

}

void AssetManager::initialize()
{
	m_basicNormalTemplate = loadTextureFromDisk("basic_normal.png", textureType::NORMALS, textureWrapMethod::REPEAT);
	m_basicAlbedoTemplate = loadTextureFromDisk("basic_albedo.png", textureType::DIFFUSE, textureWrapMethod::REPEAT);
	m_basicMetallicTemplate = loadTextureFromDisk("basic_metallic.png", textureType::SPECULAR, textureWrapMethod::REPEAT);
	m_basicRoughnessTemplate = loadTextureFromDisk("basic_roughness.png", textureType::AMBIENT, textureWrapMethod::REPEAT);
	m_basicAOTemplate = loadTextureFromDisk("basic_ao.png", textureType::EMISSIVE, textureWrapMethod::REPEAT);

	m_UnitCubeTemplate = RenderingManager::getInstance().addMeshData();
	auto lastMeshData = &RenderingManager::getInstance().getMeshData(m_UnitCubeTemplate);
	lastMeshData->addUnitCube();
	lastMeshData->setup(meshDrawMethod::TRIANGLE, false, false);
	lastMeshData->initialize();

	m_UnitSphereTemplate = RenderingManager::getInstance().addMeshData();
	lastMeshData = &RenderingManager::getInstance().getMeshData(m_UnitSphereTemplate);
	lastMeshData->addUnitSphere();
	lastMeshData->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastMeshData->initialize();

	m_UnitQuadTemplate = RenderingManager::getInstance().addMeshData();
	lastMeshData = &RenderingManager::getInstance().getMeshData(m_UnitQuadTemplate);
	lastMeshData->addUnitQuad();
	lastMeshData->setup(meshDrawMethod::TRIANGLE, true, true);
	lastMeshData->initialize();
}

void AssetManager::update()
{
}

void AssetManager::shutdown()
{
}

void AssetManager::loadAsset(const std::string & filePath)
{
	auto l_subfix = filePath.substr(filePath.find(".") + 1);
	//@TODO: generalize a loader base class 
	if (m_supportedTextureType.find(l_subfix) != m_supportedTextureType.end())
	{
		loadTextureImpl(filePath);
	}
	else if (m_supportedModelType.find(l_subfix) != m_supportedModelType.end())
	{
		loadModelImpl(filePath);
	}
	else if (m_supportedShaderType.find(l_subfix) != m_supportedShaderType.end())
	{
		loadShaderImpl(filePath);
	}
}

void AssetManager::loadAsset(const std::string & filePath, VisibleComponent & visibleComponent)
{
	auto l_subfix = filePath.substr(filePath.find(".") + 1);
	//@TODO: generalize a loader base class 
	if (m_supportedTextureType.find(l_subfix) != m_supportedTextureType.end())
	{
		loadTextureImpl(filePath, textureType::DIFFUSE, visibleComponent);
	}
	else if (m_supportedModelType.find(l_subfix) != m_supportedModelType.end())
	{
		loadModelImpl(filePath, visibleComponent);
	}
}

void AssetManager::loadAsset(const std::string & filePath, textureType textureType, VisibleComponent & visibleComponent)
{
	loadTextureImpl(filePath, textureType, visibleComponent);
}

std::string AssetManager::loadShader(const std::string & fileName) const
{
	std::ifstream file;
	file.open(("../res/shaders/" + fileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

void AssetManager::loadShaderImpl(const std::string & filePath, std::string & fileContent)
{
	std::ifstream file;
	file.open(("../res/shaders/" + filePath).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	fileContent = shaderStream.str();
	file.close();
}

void AssetManager::loadModelImpl(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedGraphicDataMap = m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedGraphicDataMap != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedGraphicDataMap->second, visibleComponent);
		LogManager::getInstance().printLog("AssetManager: " + l_convertedFilePath + " has already been loaded before, successfully assigned graphicDataMap IDs.");
	}
	else
	{
		// read file via ASSIMP
		Assimp::Importer l_assImporter;
		auto l_assScene = l_assImporter.ReadFile(m_modelRelativePath + l_convertedFilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (l_assScene == nullptr)
		{
			l_assScene = l_assImporter.ReadFile(m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
			// save model file as .innoModel binary file
			Assimp::Exporter l_assExporter;
			l_assExporter.Export(l_assScene, "assbin", m_modelRelativePath + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
			LogManager::getInstance().printLog("AssetManager: " + fileName + " is successfully converted.");
		}
		if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
		{
			LogManager::getInstance().printLog("ERROR:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
			addUnitMesh(visibleComponent, unitMeshType::CUBE);
			return;
		}
		// only need last part of file name without subfix as material's subfolder name
		auto& l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);
		auto& l_graphicDataMap = processAssimpScene(l_fileName, l_assScene, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod);

		m_loadedModelMap.emplace(l_convertedFilePath, l_graphicDataMap);
		assignloadedModel(l_graphicDataMap, visibleComponent);

		LogManager::getInstance().printLog("AssetManager: " + l_convertedFilePath + " is loaded for the first time, successfully assigned graphicDataMap IDs.");
	}
}

void AssetManager::assignloadedModel(graphicDataMap& loadedGraphicDataMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setGraphicDataMap(loadedGraphicDataMap);
	assignDefaultTextures(textureAssignType::ADD_DEFAULT, visibleComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&visibleComponent);
}

graphicDataMap AssetManager::processAssimpScene(const std::string& fileName, const aiScene* aiScene, meshDrawMethod& meshDrawMethod, textureWrapMethod& textureWrapMethod)
{
	auto l_graphicDataMap = graphicDataMap();

	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		auto& l_loadedgraphicDataMap = processAssimpNode(fileName, aiScene->mRootNode, aiScene, meshDrawMethod, textureWrapMethod);
		l_graphicDataMap.insert(l_loadedgraphicDataMap.begin(), l_loadedgraphicDataMap.end());
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			auto& l_loadedgraphicDataMap = processAssimpNode(fileName, aiScene->mRootNode->mChildren[i], aiScene, meshDrawMethod, textureWrapMethod);
			l_graphicDataMap.insert(l_loadedgraphicDataMap.begin(), l_loadedgraphicDataMap.end());
		}
	}

	return l_graphicDataMap;
}


graphicDataMap AssetManager::processAssimpNode(const std::string& fileName, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod)
{
	auto l_graphicDataMap = graphicDataMap();
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_graphicDataPair = graphicDataPair();

		l_graphicDataPair.first = processSingleAssimpMesh(scene->mMeshes[node->mMeshes[i]], meshDrawMethod);

		// process material if there was anyone
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			l_graphicDataPair.second = processSingleAssimpMaterial(fileName, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex], textureWrapMethod);
		}
		l_graphicDataMap.emplace(l_graphicDataPair);
	}
	return l_graphicDataMap;
}

meshDataID AssetManager::processSingleAssimpMesh(aiMesh * mesh, meshDrawMethod meshDrawMethod) const
{
	auto l_meshDataID = RenderingManager::getInstance().addMeshData();
	auto& lastMeshData = RenderingManager::getInstance().getMeshData(l_meshDataID);

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
			lastMeshData.getIntices().emplace_back(face.mIndices[j]);
		}
	}
	lastMeshData.setup(meshDrawMethod, false, false);
	//lastMeshData.initialize();
	LogManager::getInstance().printLog("innoMesh is loaded.");
	return l_meshDataID;
}

void AssetManager::addVertexData(aiMesh * aiMesh, int vertexIndex, MeshData& meshData) const
{
	// @TODO: should pass in a GLVertexData reference rather than wasting memory here
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
	meshData.getVertices().emplace_back(vertexData);
}

textureDataMap AssetManager::processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod)
{
	auto l_textureDataMap = textureDataMap();
	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			auto l_textureDataPair = textureDataPair();
			aiString l_AssString;
			aiMaterial->GetTexture(aiTextureType(i), 0, &l_AssString);
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
			l_textureDataPair.first = textureType(aiTextureType(i));
			l_textureDataPair.second = loadTextureFromDisk(fileName + "//" + l_localPath, textureType(aiTextureType(i)), textureWrapMethod);

			l_textureDataMap.emplace(l_textureDataPair);
		}
	}
	return l_textureDataMap;
}

void AssetManager::loadTextureImpl(const std::string & fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	auto l_loadedTextureData = m_loadedTextureMap.find(fileName);
	// check if this file has already loaded
	if (l_loadedTextureData != m_loadedTextureMap.end())
	{
		assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTextureData->second, visibleComponent);
		LogManager::getInstance().printLog("innoTexture: " + fileName + " is already loaded, successfully assigned loaded texture data IDs.");
	}
	else
	{
		auto l_textureDataPair = textureDataPair(textureType, loadTextureFromDisk(fileName, textureType, visibleComponent.m_textureWrapMethod));
		m_loadedTextureMap.emplace(fileName, l_textureDataPair);
		assignLoadedTexture(textureAssignType::OVERWRITE, l_textureDataPair, visibleComponent);
	}
}

void AssetManager::loadModelImpl(const std::string & fileName)
{
}

void AssetManager::loadTextureImpl(const std::string &filePath)
{
	auto l_loadedTextureData = m_loadedTextureMap.find(filePath);
	// check if this file has already loaded
	if (l_loadedTextureData != m_loadedTextureMap.end())
	{
		return;
	}
	else
	{
		loadTextureFromDisk(filePath);
	}
}

void AssetManager::assignLoadedTexture(textureAssignType textureAssignType, textureDataPair& loadedTextureDataPair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD_DEFAULT)
	{
		visibleComponent.addTextureData(loadedTextureDataPair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		visibleComponent.overwriteTextureData(loadedTextureDataPair);
	}
}

textureDataID AssetManager::loadTextureFromDisk(const std::string & fileName, textureType textureType, textureWrapMethod textureWrapMethod)
{
	int width, height, nrChannels;
	// load image
	stbi_set_flip_vertically_on_load(true);
	auto *data = stbi_load((m_textureRelativePath + fileName).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		auto id = RenderingManager::getInstance().addTextureData();
		auto& lastTextureData = RenderingManager::getInstance().getTextureData(id);
		lastTextureData.setup(textureType, textureWrapMethod, 0, nrChannels, width, height, data);
		lastTextureData.initialize();
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

void AssetManager::loadTextureFromDisk(const std::string & filePath)
{
	int width, height, nrChannels;
	// load image
	stbi_set_flip_vertically_on_load(true);
	auto *data = stbi_load((m_textureRelativePath + filePath).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		m_rawTextureDatas.emplace(filePath, data);
	}
	else
	{
		LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + filePath);
	}
	//stbi_image_free(data);
}

void AssetManager::loadShaderImpl(const std::string & filePath)
{
	//@TODO: such a generalized file loader!
	std::ifstream file;
	file.open((m_shaderRelativePath + filePath).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	m_rawShaderDatas.emplace(filePath, shaderStream.str());
	file.close();
}

void AssetManager::loadCubeMapTextures(const std::vector<std::string>& fileName, VisibleComponent & visibleComponent) const
{
	int width, height, nrChannels;
	for (auto i = (unsigned int)0; i < fileName.size(); i++)
	{
		// load image, do not flip texture
		stbi_set_flip_vertically_on_load(false);
		auto *data = stbi_load((m_textureRelativePath + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			auto id = RenderingManager::getInstance().addTextureData();
			auto lastTextureData = &RenderingManager::getInstance().getTextureData(id);
			lastTextureData->setup(textureType::CUBEMAP, visibleComponent.m_textureWrapMethod, i, nrChannels, width, height, data);
			visibleComponent.overwriteTextureData(textureDataPair(textureType::CUBEMAP, id));
			LogManager::getInstance().printLog("innoTexture: " + fileName[i] + " is loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[i]));
		}
		//stbi_image_free(data);
	}
}


void AssetManager::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, textureDataPair(textureType::NORMALS, m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, textureDataPair(textureType::DIFFUSE, m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, textureDataPair(textureType::SPECULAR, m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, textureDataPair(textureType::AMBIENT, m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, textureDataPair(textureType::EMISSIVE, m_basicAOTemplate), visibleComponent);
	}
}

void AssetManager::addUnitMesh(VisibleComponent & visibleComponent, unitMeshType unitMeshType)
{
	meshDataID l_UnitMeshTemmplate;
	switch (unitMeshType)
	{
	case unitMeshType::QUAD: l_UnitMeshTemmplate = m_UnitQuadTemplate; break;
	case unitMeshType::CUBE: l_UnitMeshTemmplate = m_UnitCubeTemplate; break;
	case unitMeshType::SPHERE: l_UnitMeshTemmplate = m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemmplate);
	assignDefaultTextures(textureAssignType::OVERWRITE, visibleComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&visibleComponent);
}
