#pragma once
#include "../manager/LogManager.h"
#include "../platform-dependency/GL/GLGraphicData.h"

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX };

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
	void addTestSkybox();
	void addTestBillboard();

private:
	GLMeshData m_GLMeshData;
	std::vector<GLVertexData> m_vertices;
	std::vector<unsigned int> m_intices;
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
//class ITextureData
//{
//public:
//	ITextureData();
//	virtual ~ITextureData();
//
//	enum class textureType { INVISIBLE, ALBEGO, CUBEMAP };
//
//	virtual void init() = 0;
//	virtual void update() = 0;
//	virtual void shutdown() = 0;
//
//	virtual void setTextureType(textureType textureType) = 0;
//	virtual textureType getTextureType() const = 0;
//};
//
//class GLTextureData : public ITextureData
//{
//public:
//	GLTextureData();
//	~GLTextureData();
//
//	void init() override;
//	void update() override;
//	void shutdown() override;
//
//	void setTextureType(textureType textureType) override;
//	textureType getTextureType() const override;
//
//private:
//	GLuint m_textureID;
//	textureType m_textureType;
//	void sendDataToGPU(int textureWidth, int textureHeight, unsigned char * textureData) const;
//	void sendDataToGPU(unsigned int faceCount, int cubemapTextureWidth, int cubemapTextureHeight, unsigned char * cubemapTextureData) const;
//};