#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "entity/BaseGraphicPrimitive.h"

class IAssetSystem : public ISystem
{
public:
	virtual ~IAssetSystem() {};
	virtual void loadTextureFromDisk(const std::vector<std::string>& fileName, textureType textureType, textureWrapMethod textureWrapMethod, BaseTexture* baseDexture) const = 0;
	virtual void loadModelFromDisk(const std::string & fileName, modelMap& modelMap, meshDrawMethod meshDrawMethod, textureWrapMethod textureWrapMethod) = 0;
	virtual std::string loadShader(const std::string& fileName) const = 0;
};