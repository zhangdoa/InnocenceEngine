#pragma once
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLGraphicData.h"
#include "innoMath.h"

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE };

class MeshData : public IBaseObject
{
public:
	MeshData();
	~MeshData();

	void setup() override;
	void setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents);
	void initialize() override;
	void update() override;
	void shutdown() override;

	std::vector<GLVertexData>& getVertices();
	std::vector<unsigned int>& getIntices();

	void addVertices(GLVertexData& GLVertexData);
	void addVertices(glm::vec3 & pos, glm::vec2 & texCoord, glm::vec3 & m_normal);
	void addVertices(float pos_x, float pos_y, float pos_z, float texCoord_x, float texCoord_y, float normal_x, float normal_y, float normal_z);
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
	bool m_calculateNormals;
	bool m_calculateTangents;
};

//typedef unsigned char stbi_uc;

class TextureData : public IBaseObject
{
public:
	TextureData();
	~TextureData();

	void setup() override;
	void setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData);
	void initialize() override;	
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

typedef GameObjectID textureID;
typedef GameObjectID meshID;
typedef std::pair<textureType, textureID> texturePair;
typedef std::unordered_map<textureType, textureID> textureMap;
typedef std::pair<meshID, textureMap> modelPair;
typedef std::unordered_map<meshID, textureMap> modelMap;

enum class shadowProjectionType { ORTHOGRAPHIC, PERSPECTIVE };

//class ShadowMapData
//{
//public:
//	ShadowMapData();
//	~ShadowMapData();
//
//	void init();
//	void draw();
//	void shutdown();
//
//	void setShadowProjectionType(shadowProjectionType shadowProjectionType);
//	void getProjectionMatrix(glm::mat4& projectionMatrix);
//
//private:
//	GLuint m_textureID;
//	GLuint depthMapFBO;
//	const unsigned int m_shadowMapWidth = 2048;
//	const unsigned int m_shadowMapHeight = 2048;
//
//	glm::mat4 m_projectionMatrix = glm::mat4();
//	shadowProjectionType m_shadowProjectionType = shadowProjectionType::ORTHOGRAPHIC;
//};

struct IVertex
{
	//@TODO: generalize math class
	glm::vec3 m_pos;
	glm::vec2 m_texCoord;
	glm::vec3 m_normal;
};

class IMesh : public IBaseObject
{
public:
	IMesh() {};
	virtual ~IMesh() {};

	void setup() override;
	void setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents);

	void addVertices(IVertex& IVertex);
	//void addVertices(glm::vec3 & pos, glm::vec2 & texCoord, glm::vec3 & m_normal);
	void addVertices(float pos_x, float pos_y, float pos_z, float texCoord_x, float texCoord_y, float normal_x, float normal_y, float normal_z);
	void addUnitCube();
	void addUnitSphere();
	void addUnitQuad();

	meshID getMeshDataID() const;

protected:
	std::vector<IVertex> m_vertices;
	std::vector<unsigned int> m_indices;

	meshDrawMethod m_meshDrawMethod;
	bool m_calculateNormals;
	bool m_calculateTangents;
};

class GLMesh : public IMesh
{
public:
	GLMesh() {};
	~GLMesh() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_VAO = 0;
	GLuint m_VBO = 0;
	GLuint m_IBO = 0;
};

class ITexture : public IBaseObject
{
public:
	ITexture() {};
	virtual ~ITexture() {};

	void setup() override;
	void setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureIndex, int textureFormat, int textureWidth, int textureHeight, void * textureData);

	textureID getTextureDataID() const;

protected:	
	textureType m_textureType;
	textureWrapMethod m_textureWrapMethod;

	int m_textureIndex;
	int m_textureFormat;
	int m_textureWidth;
	int m_textureHeight;
	void* m_textureRawData;
};

class GLTexture : ITexture
{
public:
	GLTexture() {};
	~GLTexture() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

class Material : public IBaseObject
{
public:
	Material();
	~Material();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	void addTextureData(textureType textureType, textureID textureDataID);

private:
	textureMap m_textureDataMap;
};

class Model : public IBaseObject
{
public:
	Model();
	~Model();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	IMesh* m_mesh;
};