#pragma once
#include "common/stdafx.h"
#include "IManager.h"
#include "entity/BaseGraphicPrimitive.h"

class IRenderingManager : public IManager
{
public:
	virtual ~IRenderingManager() {};

	virtual void render() = 0;
	virtual meshID addMesh() = 0;
	virtual textureID addTexture(textureType textureType) = 0;
	virtual BaseMesh* getMesh(meshID meshID) = 0;
	virtual BaseTexture* getTexture(textureType textureType, textureID textureID) = 0;
};