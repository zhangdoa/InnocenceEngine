#pragma once
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLGraphicData.h"

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE };

class MeshData : public IBaseObject
{
public:
	MeshData();
	~MeshData();

	void initialize() override;
	void setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents);
	void update() override;
	void shutdown() override;

	std::vector<GLVertexData>& getVertices();
	std::vector<unsigned int>& getIntices();

	void addVertices(GLVertexData& GLVertexData);
	void sendDataToGPU(bool calculateNormals, bool calculateTangents);
	void addUnitCube();
	void addUnitSphere();
	void addUnitQuad();

	void setMeshDrawMethod(meshDrawMethod meshDrawMethod);
	const meshDrawMethod& getMeshDrawMethod() const;

private:
	GLMeshData m_GLMeshData;
	std::vector<GLVertexData> m_vertices;
	std::vector<unsigned int> m_indices;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
};

typedef unsigned char stbi_uc;

class TextureData : public IBaseObject
{
public:
	TextureData();
	~TextureData();

	void initialize() override;
	void setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData);
	void update() override;
	void shutdown() override;

private:
	GLTextureData m_GLTextureData;

	textureType m_textureType;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;

	int m_textureIndex;
	int m_textureFormat;
	int m_textureWidth;
	int m_textureHeight;
	void* m_textureRawData;
};

typedef GameObjectID textureDataID;
typedef GameObjectID meshDataID;
typedef std::pair<textureType, textureDataID> textureDataPair;
typedef std::unordered_map<textureType, textureDataID> textureDataMap;
typedef std::pair<meshDataID, textureDataMap> graphicDataPair;
typedef std::unordered_map<meshDataID, textureDataMap> graphicDataMap;

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