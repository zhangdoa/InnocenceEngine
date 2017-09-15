#pragma once
#include "../manager/AssetManager.h"
#include "../interface/IVisibleGameEntity.h"
#include "../manager/CoreManager.h"

class StaticMeshComponent : public IVisibleGameEntity
{
public:
	StaticMeshComponent();
	~StaticMeshComponent();

	void render() override;

	void loadModel(const std::string& meshFileName);
	void loadTexture(const std::string& textureFileName) const;

private:
	std::vector<StaticMeshData> m_meshData;
	TextureData m_textureData;

	void initialize() override;
	void update() override;
	void shutdown() override;
};

