#pragma once
#include "interface/IObject.hpp"
#include "BaseMesh.h"
#include "BaseTexture.h"
#include "BaseShader.h"
#include "BaseShaderProgram.h"
#include "BaseFrameBuffer.h"

class IMeshRawData
{
public:
	IMeshRawData() {};
	virtual ~IMeshRawData() {};

	virtual int getNumVertices() const = 0;
	virtual int getNumFaces() const = 0;
	virtual int getNumIndicesInFace(int faceIndex) const = 0;
	virtual vec4 getVertices(unsigned int index) const = 0;
	virtual vec2 getTextureCoords(unsigned int index) const = 0;
	virtual vec4 getNormals(unsigned int index) const = 0;
	virtual int getIndices(int faceIndex, int index) const = 0;
};







