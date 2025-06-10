#include "AssimpMeshProcessor.h"
#include "AssimpUtils.h"
#include "AssimpMaterialProcessor.h"

#include "assimp/scene.h"
#include "../../Common/LogService.h"
#include "../../Common/MathHelper.h"
#include "../../Common/Randomizer.h"
#include "../../Services/AssetService.h"
#include "../../Services/ComponentManager.h"
#include "../../Services/EntityManager.h"
#include "../../Engine.h"

using namespace Inno;

MeshComponent* AssimpMeshProcessor::CreateMeshComponent(const aiScene* scene, const char* baseName, uint32_t meshIndex)
{
	auto l_mesh = scene->mMeshes[meshIndex];

	Log(Verbose, "Creating MeshComponent for: ", l_mesh->mName.C_Str());

	std::vector<Vertex> l_vertices;
	std::vector<Index> l_indices;

	size_t l_indicesCount = ConvertMeshData(l_mesh, l_vertices, l_indices);

	auto l_name = std::string(baseName) + "." + std::to_string(meshIndex) + "/";
	auto l_tempEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Frame, l_name.c_str());

	auto l_meshComponent = g_Engine->Get<ComponentManager>()->Spawn<MeshComponent>(l_tempEntity, true, ObjectLifespan::Frame);
	bool result = AssetService::Save(*l_meshComponent, l_vertices, l_indices);

	g_Engine->Get<EntityManager>()->Destroy(l_tempEntity);

	if (result)
	{
		Log(Success, "Created and saved MeshComponent: ", l_name.c_str());
		return l_meshComponent;
	}
	else
	{
		Log(Error, "Failed to save MeshComponent: ", l_name.c_str());
		return nullptr;
	}
}

size_t AssimpMeshProcessor::ConvertMeshData(const aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices)
{
	auto l_numVertices = mesh->mNumVertices;

	vertices.clear();
	vertices.reserve(l_numVertices);

	// Process vertices
	for (uint32_t i = 0; i < l_numVertices; i++)
	{
		Vertex l_vertex;

		// Position
		l_vertex.m_pos.x = mesh->mVertices[i].x;
		l_vertex.m_pos.y = mesh->mVertices[i].y;
		l_vertex.m_pos.z = mesh->mVertices[i].z;

		// Default normal
		l_vertex.m_normal.x = 0.0f;
		l_vertex.m_normal.y = 0.0f;
		l_vertex.m_normal.z = 1.0f;

		// Default tangent
		l_vertex.m_tangent.x = 1.0f;
		l_vertex.m_tangent.y = 0.0f;
		l_vertex.m_tangent.z = 0.0f;

		vertices.push_back(l_vertex);
	}

	// Texture coordinates
	if (mesh->mTextureCoords[0])
	{
		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			vertices[i].m_texCoord.x = mesh->mTextureCoords[0][i].x;
			vertices[i].m_texCoord.y = mesh->mTextureCoords[0][i].y;
		}
	}

	// Normals
	if (mesh->mNormals)
	{
		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			vertices[i].m_normal.x = mesh->mNormals[i].x;
			vertices[i].m_normal.y = mesh->mNormals[i].y;
			vertices[i].m_normal.z = mesh->mNormals[i].z;
		}
	}

	// Tangents
	if (mesh->mTangents)
	{
		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			vertices[i].m_tangent.x = mesh->mTangents[i].x;
			vertices[i].m_tangent.y = mesh->mTangents[i].y;
			vertices[i].m_tangent.z = mesh->mTangents[i].z;
		}
	}

	// Process indices
	indices.clear();
	size_t l_indicesSize = 0;

	if (mesh->mNumFaces)
	{
		// Calculate total indices needed
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace l_face = mesh->mFaces[i];
			l_indicesSize += l_face.mNumIndices;
		}

		indices.reserve(l_indicesSize);

		// Extract indices
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace l_face = mesh->mFaces[i];
			for (uint32_t j = 0; j < l_face.mNumIndices; j++)
			{
				indices.push_back(l_face.mIndices[j]);
			}
		}
	}
	else
	{
		// No faces, create sequential indices
		indices.reserve(l_numVertices);
		for (uint32_t i = 0; i < l_numVertices; i++)
		{
			indices.push_back(i);
		}
		l_indicesSize = l_numVertices;
	}

	// Process bone weights
	for (uint32_t i = 0; i < mesh->mNumBones; i++)
	{
		auto l_bone = mesh->mBones[i];

		for (uint32_t j = 0; j < l_bone->mNumWeights; j++)
		{
			auto l_vertexWeight = l_bone->mWeights[j];
			auto l_weight = l_vertexWeight.mWeight;
			auto& l_vertex = vertices[l_vertexWeight.mVertexId];

			// Only the first 2 most weighted bones will be stored
			if (l_weight > l_vertex.m_pad1[1])
			{
				// Old 1st to 2nd
				l_vertex.m_pad1[2] = l_vertex.m_pad1[0];
				l_vertex.m_pad1[3] = l_vertex.m_pad1[1];
				// New as 1st
				l_vertex.m_pad1[0] = (float)i;
				l_vertex.m_pad1[1] = l_weight;
			}
			else if (l_weight > l_vertex.m_pad1[3])
			{
				// New as 2nd
				l_vertex.m_pad1[2] = (float)i;
				l_vertex.m_pad1[3] = l_weight;
			}
		}
	}

	return l_indicesSize;
}

void AssimpMeshProcessor::ProcessAssimpBone(nlohmann::json& j, const aiMesh* mesh)
{
	for (uint32_t i = 0; i < mesh->mNumBones; i++)
	{
		nlohmann::json j_child;

		auto l_bone = mesh->mBones[i];

		j_child["Name"] = l_bone->mName.C_Str();
		j_child["ID"] = i;

		AssimpUtils::to_json(j_child["Transformation"], l_bone->mOffsetMatrix);

		j["Bones"].emplace_back(j_child);
	}
}
