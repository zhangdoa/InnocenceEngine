#pragma once
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLGraphicData.h"

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE };

class MeshData
{
public:
	MeshData();
	~MeshData();

	void init();
	void draw();
	void shutdown();

	std::vector<GLVertexData>& getVertices();
	std::vector<unsigned int>& getIntices();
	void sendDataToGPU();
	void addTestCube();
	void addTestSphere();
	void addTestSkybox();
	void addTestBillboard();

	void setMeshDrawMethod(meshDrawMethod meshDrawMethod);
	const meshDrawMethod& getMeshDrawMethod() const;

private:
	GLMeshData m_GLMeshData;
	std::vector<GLVertexData> m_vertices;
	std::vector<unsigned int> m_indices;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
};

class TextureData
{
public:
	TextureData();
	~TextureData();

	void init();
	void draw();
	void shutdown();
	void sendDataToGPU(int textureIndex, int textureFormat, int textureWidth, int textureHeight, void* textureData) const;
	void setTextureType(textureType textureType);
	textureType getTextureType() const;
	void setTextureWrapMethod(textureWrapMethod textureWrapMethod);
	const textureWrapMethod& getTextureWrapMethod() const;

private:
	GLTextureData m_GLTextureData;
	textureType m_textureType;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

class GraphicData
{
public:
	GraphicData();
	~GraphicData();

	void init();
	void draw();
	void shutdown();

	const visiblilityType& getVisiblilityType() const;
	void setVisiblilityType(visiblilityType visiblilityType);
	const textureWrapMethod& getTextureWrapMethod() const;
	void setTextureWrapMethod(textureWrapMethod textureWrapMethod);

	MeshData& getMeshData();
	std::vector<TextureData>& getTextureData();

private:
	MeshData m_meshData;
	std::vector<TextureData> m_textureData;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

enum class shadowProjectionType { ORTHOGRAPHIC, PERSPECTIVE };

class ShadowMapData
{
public:
	ShadowMapData();
	~ShadowMapData();

	void init();
	void draw();
	void shutdown();

	void setShadowProjectionType(shadowProjectionType shadowProjectionType);
	void getProjectionMatrix(glm::mat4& projectionMatrix);

private:
	GLuint m_textureID;
	GLuint depthMapFBO;
	const unsigned int m_shadowMapWidth = 2048;
	const unsigned int m_shadowMapHeight = 2048;

	glm::mat4 m_projectionMatrix = glm::mat4();
	shadowProjectionType m_shadowProjectionType = shadowProjectionType::ORTHOGRAPHIC;
};