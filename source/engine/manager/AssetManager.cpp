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

modelPointerMap AssetManager::loadModelFromDisk(const std::string & fileName) const
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
		return modelPointerMap();
	}
	// only need last part of file name without subfix as material's subfolder name
	auto& l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);
	auto& l_modelPointerMap = processAssimpScene(l_fileName, l_assScene);
	
	g_pLogManager->printLog("AssetManager: " + fileName + " is loaded for the first time, successfully assigned modelMap IDs.");
	return l_modelPointerMap;
}

modelPointerMap AssetManager::processAssimpScene(const std::string& fileName, const aiScene* aiScene) const
{
	auto l_modelPointerMap = modelPointerMap();

	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		auto& l_loadedmodelMap = processAssimpNode(fileName, aiScene->mRootNode, aiScene);
		l_modelPointerMap.insert(l_loadedmodelMap.begin(), l_loadedmodelMap.end());
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			auto& l_loadedModelPointerMap = processAssimpNode(fileName, aiScene->mRootNode->mChildren[i], aiScene);
			l_modelPointerMap.insert(l_loadedModelPointerMap.begin(), l_loadedModelPointerMap.end());
		}
	}

	return l_modelPointerMap;
}


modelPointerMap AssetManager::processAssimpNode(const std::string& fileName, aiNode * node, const aiScene * scene) const
{
	auto l_modelPointerMap = modelPointerMap();
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_modelPointerPair = modelPointerPair();
		auto l_assimpMeshData = new assimpMeshRawData();
		l_assimpMeshData->m_aiMesh = scene->mMeshes[node->mMeshes[i]];
		l_modelPointerPair.first = l_assimpMeshData;

		// process material if there was anyone
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			l_modelPointerPair.second = processSingleAssimpMaterial(fileName, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex]);
		}
		l_modelPointerMap.emplace(l_modelPointerPair);
	}
	return l_modelPointerMap;
}

void AssetManager::parseloadRawModelData(const modelPointerMap & modelPointerMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, std::vector<BaseMesh*>& baseMesh, std::vector<BaseTexture*>& baseTexture) const
{
	for (auto& l_meshRawData : modelPointerMap)
	{
		int l_meshIterator = 0;
		for (auto i = (unsigned int)0; i < l_meshRawData.first->getNumVertices(); i++)
		{
			Vertex l_Vertex;

			// positions
			auto l_vertices = l_meshRawData.first->getVertices(i);
			if (&l_vertices)
			{
				l_Vertex.m_pos.x = l_vertices.x;
				l_Vertex.m_pos.y = l_vertices.y;
				l_Vertex.m_pos.z = l_vertices.z;
			}
			else
			{
				l_Vertex.m_pos.x = 0.0f;
				l_Vertex.m_pos.y = 0.0f;
				l_Vertex.m_pos.z = 0.0f;
			}

			// texture coordinates
			auto l_textureCoords = l_meshRawData.first->getTextureCoords(i);
			if (&l_textureCoords)
			{
				l_Vertex.m_texCoord.x = l_textureCoords.x;
				l_Vertex.m_texCoord.y = l_textureCoords.y;
			}
			else
			{
				l_Vertex.m_texCoord.x = 0.0f;
				l_Vertex.m_texCoord.y = 0.0f;
			}

			// normals
			auto l_normals = l_meshRawData.first->getNormals(i);
			if (&l_normals)
			{
				l_Vertex.m_normal.x = l_normals.x;
				l_Vertex.m_normal.y = l_normals.y;
				l_Vertex.m_normal.z = l_normals.z;
			}
			else
			{
				l_Vertex.m_normal.x = 0.0f;
				l_Vertex.m_normal.y = 0.0f;
				l_Vertex.m_normal.z = 0.0f;
			}
		}

		// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (auto i = (unsigned int)0; i < l_meshRawData.first->getNumFaces(); i++)
		{
			// retrieve all indices of the face and store them in the indices vector
			for (auto j = (unsigned int)0; j < l_meshRawData.first->getNumIndicesInFace(i); j++)
			{
				baseMesh[l_meshIterator]->addIndices(l_meshRawData.first->getIndices(i, j));
			}
		}
		baseMesh[l_meshIterator]->setup(meshDrawMethod, false, false);
		baseMesh[l_meshIterator]->initialize();
		g_pLogManager->printLog("innoMesh is loaded.");

		// assign texture
		int l_textureIterator = 0;
		for (auto& l_textureFileNamePair : l_meshRawData.second)
		{		
			loadTextureFromDisk({ l_textureFileNamePair.second }, l_textureFileNamePair.first, textureWrapMethod, baseTexture[l_textureIterator]);
			l_textureIterator++;
		}
		l_meshIterator++;
	}

}

textureFileNameMap AssetManager::processSingleAssimpMaterial(const std::string& fileName, aiMaterial * aiMaterial) const
{
	auto l_textureMap = textureFileNameMap();
	for (auto i = (unsigned int)0; i < aiTextureType_UNKNOWN; i++)
	{
		if (aiMaterial->GetTextureCount(aiTextureType(i)) > 0)
		{
			auto l_texturePair = textureFileNamePair();
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
				return textureFileNameMap();
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
				return textureFileNameMap();
			}
			// load image
			l_texturePair.first = l_textureType;
			l_texturePair.second = fileName + "//" + l_localPath;

			l_textureMap.emplace(l_texturePair);
		}
	}
	return l_textureMap;
}

void AssetManager::loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, BaseTexture* baseDexture) const
{
	if (textureType == textureType::CUBEMAP)
	{
		int width, height, nrChannels;

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
				return;
			}
			//stbi_image_free(data);
		}
		baseDexture->setup(textureType::CUBEMAP, textureWrapMethod::CLAMP_TO_EDGE, nrChannels, width, height, l_3DTextureRawData, false);
		baseDexture->initialize();
		g_pLogManager->printLog("inno3DTexture is fully loaded.");
	}
	else if(textureType == textureType::EQUIRETANGULAR)
	{
		int width, height, nrChannels;
		// load image, flip texture
		stbi_set_flip_vertically_on_load(true);
		auto *data = stbi_loadf((m_textureRelativePath + fileName[0]).c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			baseDexture->setup(textureType::EQUIRETANGULAR, textureWrapMethod::CLAMP_TO_EDGE, nrChannels, width, height, { data }, false);
			baseDexture->initialize();
		g_pLogManager->printLog("inno2DHDRTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			g_pLogManager->printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[0]));
			return;
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
			baseDexture->setup(textureType, textureWrapMethod, nrChannels, width, height, { data }, true);
			baseDexture->initialize();
			g_pLogManager->printLog("inno2DTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			g_pLogManager->printLog("ERROR::STBI:: Failed to load texture: " + fileName[0]);
			return;
		}
	}
}