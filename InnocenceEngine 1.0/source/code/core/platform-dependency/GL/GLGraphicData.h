#pragma once
#include "../../manager/LogManager.h"

enum class textureType { INVISIBLE, DIFFUSE, SPECULAR, AMBIENT, EMISSIVE, HEIGHT, NORMALS, SHININESS, OPACITY, DISPLACEMENT, LIGHTMAP, REFLECTION, CUBEMAP };
enum class textureWrapMethod { CLAMPTOEDGE, REPEAT };
enum class meshDrawMethod { TRIANGLE, TRIANGLE_STRIP};

struct GLVertexData
{
	glm::vec3 m_pos;
	glm::vec2 m_texCoord;
	glm::vec3 m_normal;
	glm::vec3 m_tangent;
	glm::vec3 m_bitangent;
};

class GLMeshData
{
public:
	GLMeshData();
	~GLMeshData();

	void init();
	void draw(std::vector<unsigned int>& indices, meshDrawMethod meshDrawMethod);
	void shutdown();

	void sendDataToGPU(std::vector<GLVertexData>& vertices, std::vector<unsigned int>& indices) const;

private:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

class GLTextureData
{
public:
	GLTextureData();
	~GLTextureData();

	void init(textureType textureType, textureWrapMethod textureWrapMethod);
	void draw(textureType textureType);
	void shutdown();

	void sendDataToGPU(textureType textureType, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void* textureData) const;

private:
	GLuint m_textureID = 0;
};


