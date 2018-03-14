#include "AssetSystem.h"

void AssetSystem::setup()
{
}

void AssetSystem::initialize()
{
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
	file.open(("../res/shaders/" + fileName).c_str());
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

void AssetSystem::loadModelFromDisk(const std::string & fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod)
{
	// read file via ASSIMP
	auto l_convertedFilePath = fileName.substr(0, fileName.find(".")) + ".innoModel";
	Assimp::Importer l_assImporter;

	auto l_assScene = l_assImporter.ReadFile(m_modelRelativePath + l_convertedFilePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
	if (l_assScene == nullptr)
	{
		l_assScene = l_assImporter.ReadFile(m_modelRelativePath + fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		// save model file as .innoModel binary file
		Assimp::Exporter l_assExporter;
		l_assExporter.Export(l_assScene, "assbin", m_modelRelativePath + fileName.substr(0, fileName.find(".")) + ".innoModel", 0u, 0);
		g_pLogSystem->printLog("AssetSystem: " + fileName + " is successfully converted.");
	}
	if (l_assScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !l_assScene->mRootNode)
	{
		g_pLogSystem->printLog("ERROR:ASSIMP: " + std::string{ l_assImporter.GetErrorString() });
		return;
	}
	// only need last part of file name without subfix as material's subfolder name
	auto& l_fileName = fileName.substr(fileName.find_last_of('/') + 1, fileName.find_last_of('.') - fileName.find_last_of('/') - 1);
	processAssimpScene(l_fileName, modelMap, meshDrawMethod, textureWrapMethod, l_assScene);

	g_pLogSystem->printLog("AssetSystem: " + fileName + " is loaded for the first time, successfully assigned modelMap IDs.");
}

void AssetSystem::processAssimpScene(const std::string& fileName, modelMap & modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod, const aiScene* aiScene)
{
	//check if root node has mesh attached, btw there SHOULD NOT BE ANY MESH ATTACHED TO ROOT NODE!!!
	if (aiScene->mRootNode->mNumMeshes > 0)
	{
		processAssimpNode(fileName, modelMap, aiScene->mRootNode, aiScene, meshDrawMethod, textureWrapMethod);
	}
	for (auto i = (unsigned int)0; i < aiScene->mRootNode->mNumChildren; i++)
	{
		if (aiScene->mRootNode->mChildren[i]->mNumMeshes > 0)
		{
			processAssimpNode(fileName, modelMap, aiScene->mRootNode->mChildren[i], aiScene, meshDrawMethod, textureWrapMethod);
		}
	}
}

void AssetSystem::processAssimpNode(const std::string& fileName, modelMap & modelMap, aiNode * node, const aiScene * scene, meshDrawMethod& meshDrawMethod, textureWrapMethod textureWrapMethod)
{
	// process each mesh located at the current node
	for (auto i = (unsigned int)0; i < node->mNumMeshes; i++)
	{
		auto l_modelPair = modelPair();

		processSingleAssimpMesh(l_modelPair.first, scene->mMeshes[node->mMeshes[i]], meshDrawMethod);

		// process material if there was anyone
		if (scene->mMeshes[node->mMeshes[i]]->mMaterialIndex > 0)
		{
			processSingleAssimpMaterial(fileName, l_modelPair.second, scene->mMaterials[scene->mMeshes[node->mMeshes[i]]->mMaterialIndex], textureWrapMethod);
		}
		modelMap.emplace(l_modelPair);
	}
}


void AssetSystem::processSingleAssimpMesh(meshID& meshID, aiMesh * aiMesh, meshDrawMethod meshDrawMethod) const
{
	meshID = g_pRenderingSystem->addMesh();
	auto l_meshData = g_pRenderingSystem->getMesh(meshID);

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

		l_meshData->addVertices(l_Vertex);
	}

	// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (auto i = (unsigned int)0; i < aiMesh->mNumFaces; i++)
	{
		aiFace face = aiMesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (auto j = (unsigned int)0; j < face.mNumIndices; j++)
		{
			l_meshData->addIndices(face.mIndices[j]);
		}
	}
	l_meshData->setup(meshDrawMethod, false, false);
	l_meshData->initialize();

	g_pLogSystem->printLog("innoMesh is loaded.");
}

void AssetSystem::processSingleAssimpMaterial(const std::string& fileName, textureMap & textureMap, aiMaterial * aiMaterial, textureWrapMethod textureWrapMethod) const
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
				g_pLogSystem->printLog("inno2DTexture: " + fileName + " is unknown type!");
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
				g_pLogSystem->printLog("inno2DTexture: " + fileName + " is unsupported type!");
				return;
			}
			// load image

			auto l_textureDataID = g_pRenderingSystem->addTexture(l_textureType);
			auto l_textureData = g_pRenderingSystem->getTexture(l_textureType, l_textureDataID);

			l_texturePair.first = l_textureType;
			l_texturePair.second = l_textureDataID;
			
			loadTextureFromDisk({ fileName + "//" + l_localPath }, l_textureType, textureWrapMethod, l_textureData);

			textureMap.emplace(l_texturePair);
		}
	}
}

void AssetSystem::loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, BaseTexture* baseDexture) const
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
				g_pLogSystem->printLog("inno3DTexture: " + fileName[i] + " is loaded.");
			}
			else
			{
				g_pLogSystem->printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[i]));
				return;
			}
			//stbi_image_free(data);
		}
		baseDexture->setup(textureType::CUBEMAP, textureWrapMethod::CLAMP_TO_EDGE, nrChannels, width, height, l_3DTextureRawData, false);
		baseDexture->initialize();
		g_pLogSystem->printLog("inno3DTexture is fully loaded.");
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
		g_pLogSystem->printLog("inno2DHDRTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			g_pLogSystem->printLog("ERROR::STBI:: Failed to load texture: " + (m_textureRelativePath + fileName[0]));
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
			g_pLogSystem->printLog("inno2DTexture: " + fileName[0] + " is loaded.");
		}
		else
		{
			g_pLogSystem->printLog("ERROR::STBI:: Failed to load texture: " + fileName[0]);
			return;
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

vec3 assimpMeshRawData::getVertices(unsigned int index) const
{
	return vec3(m_aiMesh->mVertices[index].x, m_aiMesh->mVertices[index].y, m_aiMesh->mVertices[index].z);
}

vec2 assimpMeshRawData::getTextureCoords(unsigned int index) const
{
	return vec2(m_aiMesh->mTextureCoords[0][index].x, m_aiMesh->mTextureCoords[0][index].y);
}

vec3 assimpMeshRawData::getNormals(unsigned int index) const
{
	return vec3(m_aiMesh->mNormals[index].x, m_aiMesh->mNormals[index].y, m_aiMesh->mNormals[index].z);
}

int assimpMeshRawData::getIndices(int faceIndex, int index) const
{
	return m_aiMesh->mFaces[faceIndex].mIndices[index];
}
