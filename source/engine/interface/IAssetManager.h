#pragma once
#include "common/stdafx.h"
#include "IManager.h"
#include "entity/BaseGraphicPrimitive.h"

class IAssetManager : public IManager
{
public:
	virtual ~IAssetManager() {};
	virtual void loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, BaseTexture* baseDexture) const = 0;
	virtual void loadModelFromDisk(const std::string & fileName, modelMap& modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod) = 0;
	virtual std::string loadShader(const std::string& fileName) const = 0;
};