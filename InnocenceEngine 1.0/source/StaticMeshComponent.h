#pragma once
#include "GraphicData.h"
#include "IVisibleGameEntity.h"

class StaticMeshComponent : public IVisibleGameEntity
{
public:
	StaticMeshComponent();
	~StaticMeshComponent();

	void loadMesh(const std::string& meshFileName) const;
	void loadTexture(const std::string& textureFileName) const;

private:
	MeshData m_meshData;
	TextureData m_textureData;

	void init() override;
	void update() override;
	void shutdown() override;
};

