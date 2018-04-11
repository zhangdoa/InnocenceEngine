#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "entity/BaseGraphicPrimitiveHeader.h"

class IRenderingSystem : public ISystem
{
public:
	virtual ~IRenderingSystem() {};

	virtual void setWindowName(const std::string& windowName) = 0;
	virtual void render() = 0;
	virtual bool canRender() = 0;
	virtual meshID addMesh(meshType meshType) = 0;
	virtual textureID addTexture(textureType textureType) = 0;
	virtual BaseMesh* getMesh(meshType meshType, meshID meshID) = 0;
	virtual BaseTexture* getTexture(textureType textureType, textureID textureID) = 0;
	virtual void removeMesh(meshType meshType, meshID meshID) = 0;
	virtual void removeTexture(textureID textureID) = 0;
};