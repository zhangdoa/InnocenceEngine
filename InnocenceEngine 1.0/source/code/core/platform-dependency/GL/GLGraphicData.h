#pragma once
#include "../../manager/LogManager.h"

enum class textureType { INVISIBLE, DIFFUSE, SPECULAR, AMBIENT, EMISSIVE, HEIGHT, NORMALS, SHININESS, OPACITY, DISPLACEMENT, LIGHTMAP, REFLECTION, CUBEMAP };

struct GLVertexData
{
	glm::vec3 m_pos;
	glm::vec2 m_texCoord;
	glm::vec3 m_normal;
	// @TODO: tangent and bitangent
};

class GLMeshData
{
public:
	GLMeshData();
	~GLMeshData();

	void init();
	void draw(std::vector<unsigned int>& indices);
	void shutdown();

	void sendDataToGPU(std::vector<GLVertexData>& vertices, std::vector<unsigned int>& indices, bool calcNormals) const;

private:
	GLuint m_VAO;
	GLuint m_VBO;
	GLuint m_IBO;
};

class GLTextureData
{
public:
	GLTextureData();
	~GLTextureData();

	void init(textureType textureType);
	void draw(textureType textureType);
	void shutdown();

	void sendDataToGPU(textureType textureType, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void* textureData) const;

private:
	GLuint m_textureID;
};


