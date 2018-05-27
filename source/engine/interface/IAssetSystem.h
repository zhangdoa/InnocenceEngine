#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "component/MeshDataComponent.h"
#include "component/TextureDataComponent.h"

class IAssetSystem : public ISystem
{
public:
	virtual ~IAssetSystem() {};

	virtual meshID addMesh(meshType meshType) = 0;
	virtual textureID addTexture(textureType textureType) = 0;
	virtual MeshDataComponent* getMesh(meshType meshType, meshID meshID) = 0;
	virtual TextureDataComponent* getTexture(textureType textureType, textureID textureID) = 0;
	virtual void removeMesh(meshType meshType, meshID meshID) = 0;
	virtual void removeTexture(textureID textureID) = 0;

	virtual std::string loadShader(const std::string& fileName) const = 0;
};