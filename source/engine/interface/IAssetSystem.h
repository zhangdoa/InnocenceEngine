#pragma once
#include "common/stdafx.h"
#include "ISystem.h"
#include "component/MeshDataComponent.h"
#include "component/TextureDataComponent.h"

class IAssetSystem : public ISystem
{
public:
	virtual ~IAssetSystem() {};

	virtual meshID addMesh() = 0;
	virtual meshID addMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) = 0;
	virtual textureID addTexture(textureType textureType) = 0;
	virtual MeshDataComponent* getMesh(meshID meshID) = 0;
	virtual TextureDataComponent* getTexture(textureID textureID) = 0;
	virtual void removeMesh(meshID meshID) = 0;
	virtual void removeTexture(textureID textureID) = 0;
	virtual vec4 findMaxVertex(meshID meshID) = 0;
	virtual vec4 findMinVertex(meshID meshID) = 0;
	virtual std::string loadShader(const std::string& fileName) const = 0;
};

