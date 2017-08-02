#pragma once
#include "../data/GraphicData.h"
#include "../interface/IVisibleGameEntity.h"


class SkyboxComponent : public IVisibleGameEntity
{
public:
	SkyboxComponent();
	~SkyboxComponent();

	void render() override;

private:
	SkyboxMeshData m_meshData;
	CubemapData m_cubemapData;

	void init() override;
	void update() override;
	void shutdown() override;
};

