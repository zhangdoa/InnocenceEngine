#pragma once
#include "interface/IObject.hpp"
#include "innoMath.h"
#include "entity/ComponentHeaders.h""
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/IAssetSystem.h"

#define USE_OPENGL

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;
extern IAssetSystem* g_pAssetSystem;

class IMeshRawData
{
public:
	IMeshRawData() {};
	virtual ~IMeshRawData() {};

	virtual int getNumVertices() const = 0;
	virtual int getNumFaces() const = 0;
	virtual int getNumIndicesInFace(int faceIndex) const = 0;
	virtual vec3 getVertices(unsigned int index) const = 0;
	virtual vec2 getTextureCoords(unsigned int index) const = 0;
	virtual vec3 getNormals(unsigned int index) const = 0;
	virtual int getIndices(int faceIndex, int index) const = 0;
};

class IScene
{
public:
	IScene() {};
	virtual ~IScene() {};
};

typedef unsigned long int textureID;
typedef unsigned long int meshID;
typedef std::pair<textureType, textureID> texturePair;
typedef std::unordered_map<textureType, textureID> textureMap;
typedef std::pair<meshID, textureMap> modelPair;
typedef std::unordered_map<meshID, textureMap> modelMap;

typedef std::pair<textureType, std::string> textureFileNamePair;
typedef std::unordered_map<textureType, std::string> textureFileNameMap;
typedef std::pair<IMeshRawData*, textureFileNameMap> modelPointerPair;
typedef std::unordered_map<IMeshRawData*, textureFileNameMap> modelPointerMap;


class BaseMesh : public IObject
{
public:
	BaseMesh() { m_meshID = std::rand(); };
	virtual ~BaseMesh() {};

	void setup() override;
	void setup(meshType meshType, meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents);
	const objectStatus& getStatus() const override;
	meshID getMeshID();

	void addVertices(const Vertex& Vertex);
	void addVertices(const vec3 & pos, const vec2 & texCoord, const vec3 & normal);
	void addVertices(double pos_x, double pos_y, double pos_z, double texCoord_x, double texCoord_y, double normal_x, double normal_y, double normal_z);
	void addIndices(unsigned int index);

	void addUnitCube();
	void addUnitSphere();
	void addUnitQuad();

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	meshID m_meshID;
	meshType m_meshType;
	std::vector<Vertex> m_vertices;
	std::vector<unsigned int> m_indices;

	meshDrawMethod m_meshDrawMethod;
	bool m_calculateNormals;
	bool m_calculateTangents;
};

class BaseTexture : public IObject
{
public:
	BaseTexture() { m_textureID = std::rand(); };
	virtual ~BaseTexture() {};

	void setup() override;
	void setup(textureType textureType, textureColorComponentsFormat textureColorComponentsFormat, texturePixelDataFormat texturePixelDataFormat, textureWrapMethod textureWrapMethod, textureFilterMethod textureMinFilterMethod, textureFilterMethod textureMagFilterMethod, int textureWidth, int textureHeight, texturePixelDataType texturePixelDataType, const std::vector<void *>& textureData);
	virtual void update(int textureIndex) = 0;
	virtual void updateFramebuffer(int colorAttachmentIndex, int textureIndex, int mipLevel) = 0;
	const objectStatus& getStatus() const override;
	textureID getTextureID();
	int getTextureWidth() const;
	int getTextureHeight() const;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	textureID m_textureID;
	textureType m_textureType;
	textureColorComponentsFormat m_textureColorComponentsFormat;
	texturePixelDataFormat m_texturePixelDataFormat;
	textureFilterMethod m_textureMinFilterMethod;
	textureFilterMethod m_textureMagFilterMethod;
	textureWrapMethod m_textureWrapMethod;
	int m_textureWidth;
	int m_textureHeight;
	texturePixelDataType m_texturePixelDataType;
	std::vector<void *> m_textureData;
};

class BaseShader : public IObject
{
public:
	BaseShader() {};
	virtual ~BaseShader() {};

	void setup() override;
	void setup(shaderType shaderType, const std::string& shaderFilePath, const std::vector<std::string>& attributions);

	const objectStatus& getStatus() const override;

	const std::string& getShaderFilePath() const;
	const std::vector<std::string>& getAttributions() const;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	shaderType m_shaderType;
	std::string m_shaderFilePath;
	std::vector<std::string> m_attributions;
	std::string m_shaderCode;

	void parseAttribution();
};

class BaseShaderProgram : public IObject
{
public:
	BaseShaderProgram() {};
	virtual ~BaseShaderProgram() {};

	void setup() override;
	void setup(shaderTuple BaseShaders);
	void update() override;
	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap) = 0;
	
	const objectStatus& getStatus() const override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	BaseShader* m_vertexShader;
	BaseShader* m_geometryShader;
	BaseShader* m_fragmentShader;
};

class BaseFrameBufferWIP : public IObject
{
public:
	BaseFrameBufferWIP() {};
	virtual ~BaseFrameBufferWIP() {};

	void setup() override;
	void setup(frameBufferType frameBufferType, renderBufferType renderBufferType, const std::vector<vec2>& renderBufferStorageSize, const std::vector<BaseTexture*>& renderTargetTextures, const std::vector<BaseShaderProgram*>& renderTargetShaderPrograms, BaseFrameBufferWIP* previousBaseFrameBuffer);
	virtual void activeTexture(int colorAttachmentIndex, int textureIndex, int textureMipMapLevel) = 0;
	const unsigned int getRenderTargetNumber() const;
	const objectStatus& getStatus() const override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	
	frameBufferType m_frameBufferType = frameBufferType::FORWARD;
	renderBufferType m_renderBufferType = renderBufferType::DEPTH;
	std::vector<vec2> m_renderBufferStorageSize;
	std::vector<BaseTexture*> m_renderTargetTextures;
	std::vector<BaseShaderProgram*> m_shaderPrograms;
	BaseFrameBufferWIP* m_previousBaseFrameBuffer;
};

class BaseFrameBuffer : public IObject
{
public:
	BaseFrameBuffer() {};
	virtual ~BaseFrameBuffer() {};

	void setup() override;
	void setup(vec2 renderBufferStorageResolution, frameBufferType frameBufferType, renderBufferType renderBufferType, unsigned int renderTargetTextureNumber);
	const objectStatus& getStatus() const override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	vec2 m_renderBufferStorageResolution;
	frameBufferType m_frameBufferType = frameBufferType::FORWARD;
	renderBufferType m_renderBufferType = renderBufferType::DEPTH;
	unsigned int m_renderTargetTextureNumber = 0;
};

