#pragma once
#include "../manager/LogManager.h"
#include "../interface/IEntity.h"
#include "innoMath.h"

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE };
enum class textureType { INVISIBLE, NORMAL, ALBEDO, METALLIC, ROUGHNESS, AMBIENT_OCCLUSION, CUBEMAP, CUBEMAP_HDR, EQUIRETANGULAR};
enum class textureWrapMethod { CLAMP_TO_EDGE, REPEAT };
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
	void setup(textureType textureType, textureWrapMethod textureWrapMethod, int textureFormat, int textureWidth, int textureHeight, void * textureData);

protected:	
	textureType m_textureType;
	textureWrapMethod m_textureWrapMethod;

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
	void update(int textureIndex);
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

class GL2DHDRTexture : public I2DTexture
{
public:
	GL2DHDRTexture() {};
	~GL2DHDRTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
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
	void setup(textureType textureType, int textureFormat, int textureWidth, int textureHeight, const std::vector<void *>& textureData, bool generateMipMap);

protected:
	textureType m_textureType;
	int m_textureIndex;
	int m_textureFormat;
	int m_textureWidth;
	int m_textureHeight;
	bool m_generateMipMap;

	void* m_textureRawData_Right;
	void* m_textureRawData_Left;
	void* m_textureRawData_Top;
	void* m_textureRawData_Bottom;
	void* m_textureRawData_Back;
	void* m_textureRawData_Front;
};

class GL3DTexture : public I3DTexture
{
public:
	GL3DTexture() {};
	~GL3DTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
	void shutdown() override;

private:
	GLuint m_textureID = 0;
};

class GL3DHDRTexture : public I3DTexture
{
public:
	GL3DHDRTexture() {};
	~GL3DHDRTexture() {};

	void initialize() override;
	void update() override;
	void update(int textureIndex);
	void shutdown() override;

	// @TODO: need a FBO class
	void updateFramebuffer(int index, int mipLevel);

private:
	GLuint m_textureID = 0;
};