#pragma once
#include "../manager/LogManager.h"
#include "../interface/IEntity.h"
#include "innoMath.h"

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE };
enum class textureType { INVISIBLE, DIFFUSE, SPECULAR, AMBIENT, EMISSIVE, HEIGHT, NORMALS, SHININESS, OPACITY, DISPLACEMENT, LIGHTMAP, REFLECTION, CUBEMAP };
enum class textureWrapMethod { CLAMPTOEDGE, REPEAT };
enum class meshDrawMethod { TRIANGLE, TRIANGLE_STRIP };

typedef EntityID textureID;
typedef EntityID meshID;
typedef std::pair<textureType, textureID> texturePair;
typedef std::unordered_map<textureType, textureID> textureMap;
typedef std::pair<meshID, textureMap> modelPair;
typedef std::unordered_map<meshID, textureMap> modelMap;

class Vertex
{
public:
	Vertex();
	Vertex(const Vertex& rhs);
	Vertex& operator=(const Vertex& rhs);
	Vertex(const vec3& pos, const vec2& texCoord, const vec3& normal);
	~Vertex();

	vec3 m_pos;
	vec2 m_texCoord;
	vec3 m_normal;
};

class IMesh : public IEntity
{
public:
	IMesh() {};
	virtual ~IMesh() {};

	void setup() override;
	void setup(meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents);

	void addVertices(const Vertex& Vertex);
	void addVertices(const vec3 & pos, const vec2 & texCoord, const vec3 & normal);
	void addVertices(float pos_x, float pos_y, float pos_z, float texCoord_x, float texCoord_y, float normal_x, float normal_y, float normal_z);
	void addIndices(unsigned int index);

	void addUnitCube();
	void addUnitSphere();
	void addUnitQuad();

	meshID getMeshDataID() const;

protected:
	std::vector<Vertex> m_vertices;
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

class I2DTexture : public IEntity
{
public:
	I2DTexture() {};
	virtual ~I2DTexture() {};

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

class GL2DTexture : public I2DTexture
{
public:
	GL2DTexture() {};
	~GL2DTexture() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

enum class cubemapTexturePositionType{RIGHT, LEFT, TOP, BOTTOM, BACK, FRONT};

class I3DTexture : public IEntity
{
public:
	I3DTexture() {};
	virtual ~I3DTexture() {};

	void setup() override;
	void setup(const std::vector<textureID>& textures);

protected:
	textureID m_rightTexture;
	textureID m_leftTexture;
	textureID m_topTexture;
	textureID m_bottomTexture;
	textureID m_backTexture;
	textureID m_frontTexture;
};

class GL3DTexture : public I3DTexture
{
public:
	GL3DTexture() {};
	~GL3DTexture() {};

	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};