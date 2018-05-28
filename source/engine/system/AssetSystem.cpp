#include "AssetSystem.h"

void AssetSystem::setup()
{
	m_meshDataSystem = g_pMemorySystem->spawn<MeshDataSystem>();
	m_textureDataSystem = g_pMemorySystem->spawn<TextureDataSystem>();
}

void AssetSystem::initialize()
{
	loadDefaultAssets();
	loadAssetsForComponents();
	g_pLogSystem->printLog("AssetSystem has been initialized.");
}

void AssetSystem::update()
{
}

void AssetSystem::shutdown()
{
	g_pLogSystem->printLog("AssetSystem has been shutdown.");
}

std::string AssetSystem::loadShader(const std::string & fileName) const
{
	std::ifstream file;
	file.open((AssetSystemSingletonComponent::getInstance().m_shaderRelativePath + fileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

const objectStatus & AssetSystem::getStatus() const
{
	return m_objectStatus;
}

meshID AssetSystem::addMesh()
{
	MeshDataComponent* newMesh = g_pMemorySystem->spawn<MeshDataComponent>();
	auto l_meshMap = &AssetSystemSingletonComponent::getInstance().m_meshMap;
	l_meshMap->emplace(std::pair<meshID, MeshDataComponent*>(newMesh->m_meshID, newMesh));
	return newMesh->m_meshID;
}

meshID AssetSystem::addMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
{
	auto l_MeshID = addMesh();
	auto l_MeshData = getMesh(l_MeshID);

	std::for_each(vertices.begin(), vertices.end(), [&](Vertex val)
	{
		l_MeshData->m_vertices.emplace_back(val);
	});

	std::for_each(indices.begin(), indices.end(), [&](unsigned int val)
	{
		l_MeshData->m_indices.emplace_back(val);
	});

	return l_MeshID;
}

textureID AssetSystem::addTexture(textureType textureType)
{
	TextureDataComponent* newTexture = g_pMemorySystem->spawn<TextureDataComponent>();
	AssetSystemSingletonComponent::getInstance().m_textureMap.emplace(std::pair<textureID, TextureDataComponent*>(newTexture->m_textureID, newTexture));
	return newTexture->m_textureID;
}

MeshDataComponent* AssetSystem::getMesh(meshID meshID)
{
	auto l_meshMap = &AssetSystemSingletonComponent::getInstance().m_meshMap;
	return l_meshMap->find(meshID)->second;
}

TextureDataComponent * AssetSystem::getTexture(textureID textureID)
{
	return AssetSystemSingletonComponent::getInstance().m_textureMap.find(textureID)->second;
}

void AssetSystem::removeMesh(meshID meshID)
{
	auto l_meshMap = &AssetSystemSingletonComponent::getInstance().m_meshMap;
	auto l_mesh = l_meshMap->find(meshID);
	if (l_mesh != l_meshMap->end())
	{
		l_mesh->second->shutdown();
		l_meshMap->erase(meshID);
	}
}

void AssetSystem::removeTexture(textureID textureID)
{
	auto l_texture = AssetSystemSingletonComponent::getInstance().m_textureMap.find(textureID);
	if (l_texture != AssetSystemSingletonComponent::getInstance().m_textureMap.end())
	{
		l_texture->second->shutdown();
		AssetSystemSingletonComponent::getInstance().m_textureMap.erase(textureID);
	}
}

vec4 AssetSystem::findMaxVertex(meshID meshID)
{
	return m_meshDataSystem->findMaxVertex(*getMesh(meshID));
}

vec4 AssetSystem::findMinVertex(meshID meshID)
{
	return m_meshDataSystem->findMinVertex(*getMesh(meshID));
}

void AssetSystem::loadDefaultAssets()
{
	AssetSystemSingletonComponent::getInstance().m_basicNormalTemplate = addTexture(textureType::NORMAL);
	AssetSystemSingletonComponent::getInstance().m_basicAlbedoTemplate = addTexture(textureType::ALBEDO);
	AssetSystemSingletonComponent::getInstance().m_basicMetallicTemplate = addTexture(textureType::METALLIC);
	AssetSystemSingletonComponent::getInstance().m_basicRoughnessTemplate = addTexture(textureType::ROUGHNESS);
	AssetSystemSingletonComponent::getInstance().m_basicAOTemplate = addTexture(textureType::AMBIENT_OCCLUSION);

	loadTextureFromDisk({ "basic_normal.png" }, textureType::NORMAL, textureWrapMethod::REPEAT, getTexture( AssetSystemSingletonComponent::getInstance().m_basicNormalTemplate));
	loadTextureFromDisk({ "basic_albedo.png" }, textureType::ALBEDO, textureWrapMethod::REPEAT, getTexture(AssetSystemSingletonComponent::getInstance().m_basicAlbedoTemplate));
	loadTextureFromDisk({ "basic_metallic.png" }, textureType::METALLIC, textureWrapMethod::REPEAT, getTexture(AssetSystemSingletonComponent::getInstance().m_basicMetallicTemplate));
	loadTextureFromDisk({ "basic_roughness.png" }, textureType::ROUGHNESS, textureWrapMethod::REPEAT, getTexture(AssetSystemSingletonComponent::getInstance().m_basicRoughnessTemplate));
	loadTextureFromDisk({ "basic_ao.png" }, textureType::AMBIENT_OCCLUSION, textureWrapMethod::REPEAT, getTexture(AssetSystemSingletonComponent::getInstance().m_basicAOTemplate));

	AssetSystemSingletonComponent::getInstance().m_Unit3DQuadTemplate = addMesh();
	auto last3DQuadMeshData = getMesh(AssetSystemSingletonComponent::getInstance().m_Unit3DQuadTemplate);
	m_meshDataSystem->addUnitQuad(*last3DQuadMeshData);
	last3DQuadMeshData->m_meshType = meshType::THREE_DIMENSION;
	last3DQuadMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	last3DQuadMeshData->m_calculateNormals = true;
	last3DQuadMeshData->m_calculateTangents = true;

	AssetSystemSingletonComponent::getInstance().m_Unit2DQuadTemplate = addMesh();
	auto last2DQuadMeshData = getMesh(AssetSystemSingletonComponent::getInstance().m_Unit2DQuadTemplate);
	m_meshDataSystem->addUnitQuad(*last2DQuadMeshData);
	last2DQuadMeshData->m_meshType = meshType::TWO_DIMENSION;
	last2DQuadMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	last2DQuadMeshData->m_calculateNormals = false;
	last2DQuadMeshData->m_calculateTangents = false;

	AssetSystemSingletonComponent::getInstance().m_UnitCubeTemplate = addMesh();
	auto lastCubeMeshData = getMesh(AssetSystemSingletonComponent::getInstance().m_UnitCubeTemplate);
	m_meshDataSystem->addUnitCube(*lastCubeMeshData);
	lastCubeMeshData->m_meshType = meshType::THREE_DIMENSION;
	lastCubeMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	lastCubeMeshData->m_calculateNormals = false;
	lastCubeMeshData->m_calculateTangents = false;

	AssetSystemSingletonComponent::getInstance().m_UnitSphereTemplate = addMesh();
	auto lastSphereMeshData = getMesh(AssetSystemSingletonComponent::getInstance().m_UnitSphereTemplate);
	m_meshDataSystem->addUnitSphere(*lastSphereMeshData);
	lastSphereMeshData->m_meshType = meshType::THREE_DIMENSION;
	lastSphereMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastSphereMeshData->m_calculateNormals = false;
	lastSphereMeshData->m_calculateTangents = false;

	AssetSystemSingletonComponent::getInstance().m_UnitLineTemplate = addMesh();
	auto lastLineMeshData = getMesh(AssetSystemSingletonComponent::getInstance().m_UnitLineTemplate);
	m_meshDataSystem->addUnitLine(*lastLineMeshData);
	lastLineMeshData->m_meshType = meshType::THREE_DIMENSION;
	lastLineMeshData->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
	lastLineMeshData->m_calculateNormals = false;
	lastLineMeshData->m_calculateTangents = false;
}

void AssetSystem::loadAssetsForComponents()
{
	for (auto i : g_pGameSystem->getVisibleComponents())
	{
		if (i->m_visiblilityType != visiblilityType::INVISIBLE)
		{
			if (i->m_meshType == meshShapeType::CUSTOM)
			{
				if (i->m_modelFileName != "")
				{
					loadModel(i->m_modelFileName, *i);
				}
			}
			else
			{
				assignUnitMesh(i->m_meshType, *i);
				assignDefaultTextures(textureAssignType::OVERWRITE, *i);
			}
		}

		if (i->m_textureFileNameMap.size() != 0)
		{
			for (auto& j : i->m_textureFileNameMap)
			{
				loadTexture({ j.second }, j.first, *i);
			}
		}
	}

}

void AssetSystem::assignUnitMesh(meshShapeType meshType, VisibleComponent & visibleComponent)
{
	meshID l_UnitMeshTemplate;
	switch (meshType)
	{
	case meshShapeType::QUAD: l_UnitMeshTemplate = AssetSystemSingletonComponent::getInstance().m_Unit3DQuadTemplate; break;
	case meshShapeType::CUBE: l_UnitMeshTemplate = AssetSystemSingletonComponent::getInstance().m_UnitCubeTemplate; break;
	case meshShapeType::SPHERE: l_UnitMeshTemplate = AssetSystemSingletonComponent::getInstance().m_UnitSphereTemplate; break;
	}
	visibleComponent.addMeshData(l_UnitMeshTemplate);
}

void AssetSystem::assignLoadedTexture(textureAssignType textureAssignType, const texturePair& loadedtexturePair, VisibleComponent & visibleComponent)
{
	if (textureAssignType == textureAssignType::ADD)
	{
		visibleComponent.addTextureData(loadedtexturePair);
	}
	else if (textureAssignType == textureAssignType::OVERWRITE)
	{
		visibleComponent.overwriteTextureData(loadedtexturePair);
	}
}

void AssetSystem::assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent)
{
	if (visibleComponent.m_visiblilityType == visiblilityType::STATIC_MESH)
	{
		assignLoadedTexture(textureAssignType, texturePair(textureType::NORMAL, AssetSystemSingletonComponent::getInstance().m_basicNormalTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ALBEDO, AssetSystemSingletonComponent::getInstance().m_basicAlbedoTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::METALLIC, AssetSystemSingletonComponent::getInstance().m_basicMetallicTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::ROUGHNESS, AssetSystemSingletonComponent::getInstance().m_basicRoughnessTemplate), visibleComponent);
		assignLoadedTexture(textureAssignType, texturePair(textureType::AMBIENT_OCCLUSION, AssetSystemSingletonComponent::getInstance().m_basicAOTemplate), visibleComponent);
	}
}

void AssetSystem::assignLoadedModel(modelMap& loadedmodelMap, VisibleComponent & visibleComponent)
{
	visibleComponent.setModelMap(loadedmodelMap);
	assignDefaultTextures(textureAssignType::ADD, visibleComponent);
}

void AssetSystem::loadTexture(const std::vector<std::string> &fileName, textureType textureType, VisibleComponent & visibleComponent)
{
	for (auto& i : fileName)
	{
		auto l_loadedTexturePair = AssetSystemSingletonComponent::getInstance().m_loadedTextureMap.find(i);
		// check if this file has already loaded
		if (l_loadedTexturePair != AssetSystemSingletonComponent::getInstance().m_loadedTextureMap.end())
		{
			assignLoadedTexture(textureAssignType::OVERWRITE, l_loadedTexturePair->second, visibleComponent);
			g_pLogSystem->printLog("innoTexture: " + i + " is already loaded, successfully assigned loaded textureID.");
		}
		else
		{
			auto l_textureID = addTexture(textureType);
			auto l_baseTexture = getTexture(l_textureID);
			loadTextureFromDisk({ i }, textureType, visibleComponent.m_textureWrapMethod, l_baseTexture);
			AssetSystemSingletonComponent::getInstance().m_loadedTextureMap.emplace(i, texturePair(textureType, l_textureID));
			assignLoadedTexture(textureAssignType::OVERWRITE, texturePair(textureType, l_textureID), visibleComponent);
		}
	}
}

void AssetSystem::loadModel(const std::string & fileName, VisibleComponent & visibleComponent)
{
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	// check if this file has already been loaded once
	auto l_loadedmodelMap = AssetSystemSingletonComponent::getInstance().m_loadedModelMap.find(l_convertedFilePath);
	if (l_loadedmodelMap != AssetSystemSingletonComponent::getInstance().m_loadedModelMap.end())
	{
		assignLoadedModel(l_loadedmodelMap->second, visibleComponent);

		g_pLogSystem->printLog("innoMesh: " + l_convertedFilePath + " is already loaded, successfully assigned loaded modelMap.");
	}
	else
	{
		modelMap l_modelMap;
		loadModelFromDisk(fileName, l_modelMap, visibleComponent.m_meshDrawMethod, visibleComponent.m_textureWrapMethod, visibleComponent.m_caclNormal);
		assignLoadedModel(l_modelMap, visibleComponent);

		//mark as loaded
		AssetSystemSingletonComponent::getInstance().m_loadedModelMap.emplace(l_convertedFilePath, l_modelMap);
	}
}

void AssetSystem::loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, TextureDataComponent* baseTexture)
{
	if (textureType == textureType::CUBEMAP)
	{
		int width, height, nrChannels;

		std::vector<void*> l_3DTextureRawData;

		for (auto i = (unsigned int)0; i < fileName.size(); i++)
		{
			// load image, do not flip texture
			stbi_set_flip_vertically_on_load(false);
			auto *data = stbi_load((AssetSystemSingletonComponent::getInstance().m_textureRelativePath + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				l_3DTextureRawData.emplace_back(data);
				g_pLogSystem->printLog("innoTexture: " + fileName[i] + " is loaded.");
			}
			else
			{
				g_pLogSystem->printLog("ERROR::STBI:: Failed to load texture: " + (AssetSystemSingletonComponent::getInstance().m_textureRelativePath + fileName[i]));
				return;
			}
			//stbi_image_free(data);
		}

		baseTexture->m_textureType = textureType::CUBEMAP;
		baseTexture->m_textureColorComponentsFormat = textureColorComponentsFormat::SRGB;
		baseTexture->m_texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
		baseTexture->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
		baseTexture->m_textureMinFilterMethod = textureFilterMethod::LINEAR;
		baseTexture->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
		baseTexture->m_textureWidth = width;
		baseTexture->m_textureHeight = height;
		baseTexture->m_texturePixelDataType = texturePixelDataType::UNSIGNED_BYTE;
		baseTexture->m_textureData = l_3DTextureRawData;

		g_pLogSystem->printLog("innoTexture: cubemap texture is fully loaded.");
	}
	else if (textureType == textureType::EQUIRETANGULAR)
	{
		int width, height, nrChannels;
		// load image, flip texture
		stbi_set_flip_vertically_on_load(true);
		auto *data = stbi_loadf((AssetSystemSingletonComponent::getInstance().m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			baseTexture->m_textureType = textureType::EQUIRETANGULAR;
			baseTexture->m_textureColorComponentsFormat = textureColorComponentsFormat::RGB16F;
			baseTexture->m_texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
			baseTexture->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
			baseTexture->m_textureMinFilterMethod = textureFilterMethod::LINEAR;
			baseTexture->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
			baseTexture->m_textureWidth = width;
			baseTexture->m_textureHeight = height;
			baseTexture->m_texturePixelDataType = texturePixelDataType::FLOAT;
			baseTexture->m_textureData = { data };

			g_pLogSystem->printLog("innoTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			g_pLogSystem->printLog("ERROR::STBI:: Failed to load texture: " + (AssetSystemSingletonComponent::getInstance().m_textureRelativePath + fileName[0]));
			return;
		}
	}
	else
	{
		int width, height, nrChannels;
		// load image
		stbi_set_flip_vertically_on_load(true);

		auto *data = stbi_load((AssetSystemSingletonComponent::getInstance().m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			baseTexture->m_textureType = textureType;
			baseTexture->m_textureColorComponentsFormat = textureColorComponentsFormat::RGB;
			baseTexture->m_texturePixelDataFormat = texturePixelDataFormat(nrChannels - 1);
			baseTexture->m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
			baseTexture->m_textureMinFilterMethod = textureFilterMethod::LINEAR_MIPMAP_LINEAR;
			baseTexture->m_textureMagFilterMethod = textureFilterMethod::LINEAR;
			baseTexture->m_textureWidth = width;
			baseTexture->m_textureHeight = height;
			baseTexture->m_texturePixelDataType = texturePixelDataType::FLOAT;
			baseTexture->m_textureData = { data };

			g_pLogSystem->printLog("innoTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			g_pLogSystem->printLog("ERROR::STBI:: Failed to load texture: " + fileName[0]);
			return;
		}
	}
}

void AssetSystem::loadModelFromDisk(const std::string & fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal)
{
	// read file via ASSIMP
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";

	Assimp::Importer l_assImporter;
	const aiScene* l_assScene;

	if (std::experimental::filesystem::exists(std::experimental::filesystem::path(AssetSystemSingletonComponent::getInstance().m_modelRelativePath + l_convertedFilePath)))
	{
		l_assScene = l_assImporter.ReadFile(AssetSystemSingletonComponent::getInstance().m_modelRelativePath + l_convertedFilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	}
	else if (std::experimental::filesystem::exists(std::experimental::filesystem::path(AssetSystemSingletonComponent::getInstance().m_modelRelativePath + fileName)))
	{
		l_assScene = l_assImporter.ReadFile(AssetSystemSingletonComponent::getInstance().m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// save model file as .innoModel binary file
		Assimp::Exporter l_assExporter;
		l_assExporter.Export(l_assScene, "assbin", AssetSystemSingletonComponent::getInstance().m_modelRelativePath + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
		g_pLogSystem->printLog("AssetSystem: " + fileName + " is successfully converted.");
	}
	else
	{
		g_pLogSystem->printLog("AssetSystem: " + fileName + " doesn't exist!");
		return;
	}

	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		g_pLogSystem->printLog("ERROR:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}

	// only need last part of file name without subfix as material's subfolder name
	auto l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);
	processAssimpScene(l_fileName, modelMap, meshDrawMethod, textureWrapMethod, l_assScene, caclNormal);

	g_pLogSystem->printLog("AssetSystem: " + fileName + " is loaded for the first time, successfully assigned modelMap IDs.");
}

void AssetSystem::processAssimpScene(const std::string& fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, const aiScene* aiScene, bool caclNormal)
{
	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		processAssimpNode(fileName, modelMap, aiScene->mRootNode, aiScene, meshDrawMethod, textureWrapMethod, caclNormal);
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			processAssimpNode(fileName, modelMap, aiScene->mRootNode->mChildren[i], aiScene, meshDrawMethod, textureWrapMethod, caclNormal);
		}
	}
}

void AssetSystem::processAssimpNode(const std::string& fileName, modelMap & modelMap, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod, bool caclNormal)
{
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_modelPair = modelPair();

		processSingleAssimpMesh(fileName, l_modelPair.first, scene->mMeshes[node->mMeshes[i]], meshDrawMethod, caclNormal);

		// process material if there was anyone
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			processSingleAssimpMaterial(fileName, l_modelPair.second, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex], textureWrapMethod);
		}
		modelMap.emplace(l_modelPair);
	}
}

void AssetSystem::processSingleAssimpMesh(const std::string& fileName, meshID& meshID, aiMesh * aiMesh, meshDrawMethod meshDrawMethod, bool caclNormal)
{
	meshID = addMesh();
	auto l_meshData = getMesh(meshID);

	for (auto i = (unsigned int)0; i < aiMesh->mNumVertices; i++)
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
			l_Vertex.m_normal.z = 0.0f;
		}
		l_meshData->m_vertices.emplace_back(l_Vertex);
	}

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (auto i = (unsigned int)0; i < aiMesh->mNumFaces; i++)
	{
		aiFace face = aiMesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = (unsigned int)0; j < face.mNumIndices; j++)
		{
			l_meshData->m_indices.emplace_back(face.mIndices[j]);
		}
	}

	l_meshData->m_meshType = meshType::THREE_DIMENSION;
	l_meshData->m_meshDrawMethod = meshDrawMethod;
	l_meshData->m_calculateNormals = caclNormal;
	l_meshData->m_calculateTangents = false;

	g_pLogSystem->printLog("innoMesh: mesh of model " + fileName + " is loaded.");
}

void AssetSystem::processSingleAssimpMaterial(const std::string& fileName, textureMap & textureMap, const aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod)
{
	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			auto l_texturePair = texturePair();
			aiString l_AssString;
			aiMaterial->GetTexture(aiTextureType(i), 0, &l_AssString);
			// set local path, remove slash
			std::string l_localPath;
			auto l_AssString_char = std::string(l_AssString.C_Str());
			if (l_AssString_char.find_last_of('//') != std::string::npos)
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
				g_pLogSystem->printLog("innoTexture: " + fileName + " is unknown type!");
				return;
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
				g_pLogSystem->printLog("innoTexture: " + fileName + " is unsupported type!");
				return;
			}
			// load image
			auto l_loadedTexturePair = AssetSystemSingletonComponent::getInstance().m_loadedTexture.find(fileName + "//" + l_localPath);
			if (l_loadedTexturePair != AssetSystemSingletonComponent::getInstance().m_loadedTexture.end())
			{
				textureMap.emplace(l_loadedTexturePair->second);
				g_pLogSystem->printLog("innoTexture: " + fileName + " is already loaded.");
			}
			else
			{
				auto l_textureDataID = addTexture(l_textureType);
				auto l_textureData = getTexture(l_textureDataID);

				l_texturePair.first = l_textureType;
				l_texturePair.second = l_textureDataID;

				loadTextureFromDisk({ fileName + "//" + l_localPath }, l_textureType, textureWrapMethod, l_textureData);

				textureMap.emplace(l_texturePair);
				AssetSystemSingletonComponent::getInstance().m_loadedTexture.emplace(fileName + "//" + l_localPath, l_texturePair);
			}
		}
	}
}

int assimpMeshRawData::getNumVertices() const
{
	return m_aiMesh->mNumVertices;
}

int assimpMeshRawData::getNumFaces() const
{
	return m_aiMesh->mNumFaces;
}

int assimpMeshRawData::getNumIndicesInFace(int faceIndex) const
{
	return m_aiMesh->mFaces[faceIndex].mNumIndices;
}

vec4 assimpMeshRawData::getVertices(unsigned int index) const
{
	return vec4(m_aiMesh->mVertices[index].x, m_aiMesh->mVertices[index].y, m_aiMesh->mVertices[index].z, 1.0);
}

vec2 assimpMeshRawData::getTextureCoords(unsigned int index) const
{
	return vec2(m_aiMesh->mTextureCoords[0][index].x, m_aiMesh->mTextureCoords[0][index].y);
}

vec4 assimpMeshRawData::getNormals(unsigned int index) const
{
	return vec4(m_aiMesh->mNormals[index].x, m_aiMesh->mNormals[index].y, m_aiMesh->mNormals[index].z, 0.0);
}

int assimpMeshRawData::getIndices(int faceIndex, int index) const
{
	return m_aiMesh->mFaces[faceIndex].mIndices[index];
}
