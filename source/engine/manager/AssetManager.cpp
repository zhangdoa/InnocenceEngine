#include "AssetManager.h"

void AssetManager::setup()
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

modelMap AssetManager::loadModelFromDisk(const std::string & fileName, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod)
{
	// read file via ASSIMP
	Assimp::Importer l_assImporter;
	auto l_assScene = l_assImporter.ReadFile(m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	if (l_assScene == nullptr)
	{
		l_assScene = l_assImporter.ReadFile(m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// save model file as .innoModel binary file
		Assimp::Exporter l_assExporter;
		l_assExporter.Export(l_assScene, "assbin", m_modelRelativePath + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
		g_pLogManager->printLog("AssetManager: " + fileName + " is successfully converted.");
	}
	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		g_pLogManager->printLog("ERROR:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return modelMap();
	}
	// only need last part of file name without subfix as material's subfolder name
	auto& l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);
	auto& l_modelMap = processAssimpScene(l_fileName, l_assScene, meshDrawMethod, textureWrapMethod);
	
	g_pLogManager->printLog("AssetManager: " + fileName + " is loaded for the first time, successfully assigned modelMap IDs.");
	return l_modelMap;
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
	auto l_meshDataID = g_pRenderingManager->addMesh();
	auto lastMeshData = g_pRenderingManager->getMesh(l_meshDataID);

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
	g_pLogManager->printLog("innoMesh is loaded.");
	return l_meshDataID;
}

void AssetManager::addVertex(aiMesh * aiMesh, int vertexIndex, BaseMesh * mesh) const
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
				g_pLogManager->printLog("inno2DTexture: " + fileName + " is unknown type!");
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
				g_pLogManager->printLog("inno2DTexture: " + fileName + " is unsupported type!");
				return textureMap();
			}
			// load image
			l_texturePair.first = l_textureType;
			l_texturePair.second = loadTextureFromDisk({ fileName + "//" + l_localPath }, l_textureType, textureWrapMethod);

			l_textureMap.emplace(l_texturePair);
		}
	}
	return l_textureMap;
}

textureID AssetManager::loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod) const
{
	if (textureType == textureType::CUBEMAP)
	{
		int width, height, nrChannels;
		auto id = g_pRenderingManager->add3DTexture();
		auto lastTextureData = g_pRenderingManager->get3DTexture(id);

		std::vector<void*> l_3DTextureRawData;

		for (auto i = (unsigned int)0; i < fileName.size(); i++)
		{
			// load image, do not flip texture
			stbi_set_flip_vertically_on_load(false);
			auto *data = stbi_load((m_textureRelativePath + fileName[i]).c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				l_3DTextureRawData.emplace_back(data);
				g_pLogManager->printLog("inno3DTexture: " + fileName[i] + " is loaded.");
			}
			else
			{
				g_pLogManager->printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[i]));
				return 0;
			}
			//stbi_image_free(data);
		}
		lastTextureData->setup(textureType::CUBEMAP, nrChannels, width, height, l_3DTextureRawData, false);
		lastTextureData->initialize();
		g_pLogManager->printLog("inno3DTexture is fully loaded.");
		return id;
	}
	else if(textureType == textureType::EQUIRETANGULAR)
	{
		int width, height, nrChannels;
		// load image, flip texture
		stbi_set_flip_vertically_on_load(true);
		auto *data = stbi_loadf((m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			auto id = g_pRenderingManager->add2DHDRTexture();
			auto last2DTextureData = g_pRenderingManager->get2DHDRTexture(id);
			last2DTextureData->setup(textureType::EQUIRETANGULAR, textureWrapMethod::CLAMP_TO_EDGE, nrChannels, width, height, data);
			last2DTextureData->initialize();

			g_pLogManager->printLog("inno2DHDRTexture: " + fileName[0] + " is loaded.");
			return id;
		}
		else
		{
			g_pLogManager->printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[0]));
			return 0;
		}
	}
	else
	{
		int width, height, nrChannels;
		// load image
		stbi_set_flip_vertically_on_load(true);

		auto *data = stbi_load((m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			auto id = g_pRenderingManager->add2DTexture();
			auto last2DTextureData = g_pRenderingManager->get2DTexture(id);
			last2DTextureData->setup(textureType, textureWrapMethod, nrChannels, width, height, data);
			last2DTextureData->initialize();
			g_pLogManager->printLog("inno2DTexture: " + fileName[0] + " is loaded.");
			return id;
		}
		else
		{
			g_pLogManager->printLog("ERROR::STBI:: Failed to load texture: " + fileName[0]);
			return 0;
		}
	}
}