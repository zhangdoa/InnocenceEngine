
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

void AssetManager::loadModel(const std::string& fileName) const
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
	// process ASSIMP's root node recursively
	processAssimpNode(l_assScene->mRootNode, l_assScene, fileName.substr(0, fileName.find(".")) + "_r");
	// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
	for (auto i = 0; i < l_assScene->mRootNode->mNumChildren; i++)
	{
		processAssimpNode(l_assScene->mRootNode->mChildren[i], l_assScene, fileName.substr(0, fileName.find(".")) + "_c" + std::to_string(i));
	}
	LogManager::getInstance().printLog("Model imported.");
}

void AssetManager::processAssimpNode(aiNode * node, const aiScene * scene, const std::string& fileName) const
{
	// process each mesh located at the current node
	for (auto i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene. 
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		//TODO: there should be lots of seperate mesh file 
		processAssimpMesh(mesh, scene, fileName + "_m" + std::to_string(i));
	}
}

void AssetManager::processAssimpMesh(aiMesh *mesh, const aiScene * scene, const std::string& fileName) const
{
	std::ofstream file("../res/models/" + fileName + ".innoMesh");

	if (file.is_open())
	{
		// Walk through each of the mesh's vertices
		file << "#INNO_MESH_VERTICES#" << std::endl;
		for (auto i = 0; i < mesh->mNumVertices; i++)
		{
			// positions
			file << mesh->mVertices[i].x << " ";
			file << mesh->mVertices[i].y << " ";
			file << mesh->mVertices[i].z << " ";

			// texture coordinates
			if (mesh->mTextureCoords[0])
			{
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				file << mesh->mTextureCoords[0][i].x << " ";
				file << mesh->mTextureCoords[0][i].y << " ";
			}
			else
			{
				file << 0.0f << " ";
				file << 0.0f << " ";
			}

			// normals
			if (mesh->mNormals)
			{
				file << mesh->mNormals[i].x << " ";
				file << mesh->mNormals[i].y << " ";
				file << mesh->mNormals[i].z << " " << std::endl;
			}
			else 
			{
				file << 0.0f << " ";
				file << 0.0f << " ";
				file << 0.0f << " " << std::endl;
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
		}

		// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		file << "#INNO_MESH_INDICES#" << std::endl;
		for (auto i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (auto j = 0; j < face.mNumIndices; j++)
			{
				file << face.mIndices[j] << std::endl;
			}
		}
		file << "#INNO_MESH_EOF#";

		file.flush();

		file.close();

		LogManager::getInstance().printLog("innoMesh created.");
	}
	else 
	{
		LogManager::getInstance().printLog("Error: Cannot inport model!");
	}
}
