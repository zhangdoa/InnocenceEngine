#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "entity/BaseGraphicPrimitive.h"

class IRenderingSystem : public ISystem
{
public:
	virtual ~IRenderingSystem() {};

	virtual void setWindowName(const std::string& windowName) = 0;
	virtual void render() = 0;
	virtual meshID addMesh() = 0;
	virtual textureID addTexture(textureType textureType) = 0;
	virtual BaseMesh* getMesh(meshID meshID) = 0;
	virtual BaseTexture* getTexture(textureType textureType, textureID textureID) = 0;
};