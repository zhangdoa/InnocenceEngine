
#include "../../main/stdafx.h"
#include "AssetManager.h"
#define STB_IMAGE_IMPLEMENTATION    
#include "../third-party/stb_image.h"

void AssetManager::setup()
{

}

void AssetManager::initialize()
{
	m_basicNormalTemplate = load2DTextureFromDisk("basic_normal.png", textureType::NORMAL, textureWrapMethod::REPEAT);
	m_basicAlbedoTemplate = load2DTextureFromDisk("basic_albedo.png", textureType::ALBEDO, textureWrapMethod::REPEAT);
	m_basicMetallicTemplate = load2DTextureFromDisk("basic_metallic.png", textureType::METALLIC, textureWrapMethod::REPEAT);
	m_basicRoughnessTemplate = load2DTextureFromDisk("basic_roughness.png", textureType::ROUGHNESS, textureWrapMethod::REPEAT);
	m_basicAOTemplate = load2DTextureFromDisk("basic_ao.png", textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT);

	m_UnitCubeTemplate = RenderingManager::getInstance().addMesh();
	auto lastMeshData = RenderingManager::getInstance().getMesh(m_UnitCubeTemplate);
	lastMeshData->addUnitCube();
	lastMeshData->setup(meshDrawMethod::TRIANGLE, false, false);
	lastMeshData->initialize();

	m_UnitSphereTemplate = RenderingManager::getInstance().addMesh();
	lastMeshData = RenderingManager::getInstance().getMesh(m_UnitSphereTemplate);
	lastMeshData->addUnitSphere();
	lastMeshData->setup(meshDrawMethod::TRIANGLE_STRIP, false, false);
	lastMeshData->initialize();

	m_UnitQuadTemplate = RenderingManager::getInstance().addMesh();
	lastMeshData = RenderingManager::getInstance().getMesh(m_UnitQuadTemplate);
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
		loadTextureImpl(filePath, textureType::ALBEDO, visibleComponent);
	}
	else if (m_supportedModelType.find(l_subfix) != m_supportedModelType.end())
	{
		loadModelImpl(filePath, visibleComponent);
	}
}

void AssetManager::loadAsset(const std::vector<std::string>& filePath, VisibleComponent & visibleComponent)
{
	load3DTextureFromDisk(filePath, visibleComponent);
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
	auto l_loadedmodelMap = m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != m_loadedModelMap.end())
	{
		assignloadedModel(l_loadedmodelMap->second, visibleComponent);
		LogManager::getInstance().printLog("AssetManager: " + l_convertedFilePath + " has already been loaded before, successfully assigned modelMap IDs.");
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
		auto& l_modelMap = processAssimpScene(l_fileName, l_assScene, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod);

		m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
		assignloadedModel(l_modelMap, visibleComponent);

		LogManager::getInstance().printLog("AssetManager: " + l_convertedFilePath + " is loaded for the first time, successfully assigned modelMap IDs.");
	}
}

void AssetManager::assignloadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setModelMap(loadedmodelMap);
	assignDefaultTextures(textureAssignType::ADD_DEFAULT, visibleComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&visibleComponent);
}

modelMap AssetManager::processAssimpScene(const std::string& fileName, const aiScene* aiScene, meshDrawMethod& meshDrawMethod, textureWrapMethod& textureWrapMethod)
{
	auto l_modelMap = modelMap();

	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		auto& l_loadedmodelMap = processAssimpNode(fileName, aiScene->mRootNode, aiScene, meshDrawMethod, textureWrapMethod);
		l_modelMap.insert(l_loadedmodelMap.begin(), l_loadedmodelMap.end());
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			auto& l_loadedmodelMap = processAssimpNode(fileName, aiScene->mRootNode->mChildren[i], aiScene, meshDrawMethod, textureWrapMethod);
			l_modelMap.insert(l_loadedmodelMap.begin(), l_loadedmodelMap.end());
		}
	}

	return l_modelMap;
}


modelMap AssetManager::processAssimpNode(const std::string& fileName, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod)
{
	auto l_modelMap = modelMap();
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_modelPair = modelPair();

		l_modelPair.first = processSingleAssimpMesh(scene->mMeshes[node->mMeshes[i]], meshDrawMethod);

		// process material if there was anyone
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			l_modelPair.second = processSingleAssimpMaterial(fileName, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex], textureWrapMethod);
		}
		l_modelMap.emplace(l_modelPair);
	}
	return l_modelMap;
}

meshID AssetManager::processSingleAssimpMesh(aiMesh * mesh, meshDrawMethod meshDrawMethod) const
{
	auto l_meshDataID = RenderingManager::getInstance().addMesh();
	auto lastMeshData = RenderingManager::getInstance().getMesh(l_meshDataID);

	for (auto i = (unsigned int)0; i < mesh->mNumVertices; i++)
	{
		addVertex(mesh, i, lastMeshData);
	}

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (auto i = (unsigned int)0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = (unsigned int)0; j < face.mNumIndices; j++)
		{
			lastMeshData->addIndices(face.mIndices[j]);
		}
	}
	lastMeshData->setup(meshDrawMethod, false, false);
	lastMeshData->initialize();
	LogManager::getInstance().printLog("innoMesh is loaded.");
	return l_meshDataID;
}

void AssetManager::addVertex(aiMesh * aiMesh, int vertexIndex, IMesh * mesh) const
{
	Vertex l_Vertex;

	// positions
	if (&aiMesh->mVertices[vertexIndex] != nullptr)
	{
		l_Vertex.m_pos.x = aiMesh->mVertices[vertexIndex].x;
		l_Vertex.m_pos.y = aiMesh->mVertices[vertexIndex].y;
		l_Vertex.m_pos.z = aiMesh->mVertices[vertexIndex].z;
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
		l_Vertex.m_texCoord.x = aiMesh->mTextureCoords[0][vertexIndex].x;
		l_Vertex.m_texCoord.y = aiMesh->mTextureCoords[0][vertexIndex].y;
	}
	else
	{
		l_Vertex.m_texCoord.x = 0.0f;
		l_Vertex.m_texCoord.y = 0.0f;
	}

	// normals
	if (aiMesh->mNormals)
	{
		l_Vertex.m_normal.x = aiMesh->mNormals[vertexIndex].x;
		l_Vertex.m_normal.y = aiMesh->mNormals[vertexIndex].y;
		l_Vertex.m_normal.z = aiMesh->mNormals[vertexIndex].z;
	}
	else
	{
		l_Vertex.m_normal.x = 0.0f;
		l_Vertex.m_normal.y = 0.0f;
		l_Vertex.m_normal.z = 0.0f;
	}

	mesh->addVertices(l_Vertex);
}

textureMap AssetManager::processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod)
{
	auto l_textureMap = textureMap();
	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			auto l_texturePair = texturePair();
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

			textureType l_textureType;

			if (aiTextureType(i) == aiTextureType::aiTextureType_NONE)
			{
				LogManager::getInstance().printLog("inno2DTexture: " + fileName + " is unknown type!");
				return textureMap();
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_NORMALS)
			{
				l_textureType = textureType::NORMAL;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_DIFFUSE)
			{
				l_textureType = textureType::ALBEDO;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_SPECULAR)
			{
				l_textureType = textureType::METALLIC;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_AMBIENT)
			{
				l_textureType = textureType::ROUGHNESS;
			}
			else if (aiTextureType(i) == aiTextureType::aiTextureType_EMISSIVE)
			{
				l_textureType = textureType::AMBIENT_OCCLUSION;
			}
			else
			{
				LogManager::getInstance().printLog("inno2DTexture: " + fileName + " is unsupported type!");
				return textureMap();
			}
			// load image
			l_texturePair.first = l_textureType;
			l_texturePair.second = load2DTextureFromDisk(fileName + "//" + l_localPath, l_textureType, textureWrapMethod);

			l_textureMap.emplace(l_texturePair);
		}
	}
	return l_textureMap;
}

void AssetManager::loadTextureImpl(const std::string & fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	auto l_loadedTextureData = m_loadedTextureMap.find(fileName);
	// check if this file has already loaded
	if (l_loadedTextureData != m_loadedTextureMap.end())
	{
		assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTextureData->second, visibleComponent);
		LogManager::getInstance().printLog("inno2DTexture: " + fileName + " is already loaded, successfully assigned loaded texture data IDs.");
	}
	else
	{
		auto l_texturePair = texturePair(textureType, load2DTextureFromDisk(fileName, textureType, visibleComponent.m_textureWrapMethod));
		m_loadedTextureMap.emplace(fileName, l_texturePair);
		assignLoadedTexture(textureAssignType::OVERWRITE, l_texturePair, visibleComponent);
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
		load2DTextureFromDisk(filePath);
	}
}

void AssetManager::assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD_DEFAULT)
	{
		visibleComponent.addTextureData(loadedtexturePair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		visibleComponent.overwriteTextureData(loadedtexturePair);
	}
}

textureID AssetManager::load2DTextureFromDisk(const std::string & fileName, textureType textureType, textureWrapMethod textureWrapMethod)
{
	int width, height, nrChannels;
	// load image
	stbi_set_flip_vertically_on_load(true);
	auto *data = stbi_load((m_textureRelativePath + fileName).c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		auto id = RenderingManager::getInstance().add2DTexture();
		auto lastTextureData = RenderingManager::getInstance().get2DTexture(id);
		lastTextureData->setup(textureType, textureWrapMethod, nrChannels, width, height, data);
		lastTextureData->initialize();
		LogManager::getInstance().printLog("inno2DTexture: " + fileName + " is loaded.");
		return id;
	}
	else
	{
		LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + fileName);
		return 0;
	}
	//stbi_image_free(data);
}

void AssetManager::load2DTextureFromDisk(const std::string & filePath)
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

void AssetManager::load3DTextureFromDisk(const std::vector<std::string>& fileName, VisibleComponent & visibleComponent) const
{
	int width, height, nrChannels;
	auto id = RenderingManager::getInstance().add3DTexture();
	auto lastTextureData = RenderingManager::getInstance().get3DTexture(id);

	std::vector<void*> l_3DTextureRawData;

	for (auto i = (unsigned int)0; i < fileName.size(); i++)
	{
		// load image, do not flip texture
		stbi_set_flip_vertically_on_load(false);
		auto *data = stbi_load((m_textureRelativePath + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			l_3DTextureRawData.emplace_back(data);
			LogManager::getInstance().printLog("inno3DTexture: " + fileName[i] + " is loaded.");
		}
		else
		{
			LogManager::getInstance().printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[i]));
		}
		//stbi_image_free(data);
	}
	lastTextureData->setup(nrChannels, width, height, l_3DTextureRawData);
	lastTextureData->initialize();
	visibleComponent.addTextureData(texturePair(textureType::CUBEMAP, id));
	LogManager::getInstance().printLog("inno3DTexture is fully loaded.");
}


void AssetManager::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, texturePair(textureType::NORMAL, m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ALBEDO, m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::METALLIC, m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ROUGHNESS, m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::AMBIENT_OCCLUSION, m_basicAOTemplate), visibleComponent);
	}
}

void AssetManager::addUnitMesh(VisibleComponent & visibleComponent, unitMeshType unitMeshType)
{
	meshID l_UnitMeshTemplate;
	switch (unitMeshType)
	{
	case unitMeshType::QUAD: l_UnitMeshTemplate = m_UnitQuadTemplate; break;
	case unitMeshType::CUBE: l_UnitMeshTemplate = m_UnitCubeTemplate; break;
	case unitMeshType::SPHERE: l_UnitMeshTemplate = m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemplate);
	assignDefaultTextures(textureAssignType::OVERWRITE, visibleComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&visibleComponent);
}
