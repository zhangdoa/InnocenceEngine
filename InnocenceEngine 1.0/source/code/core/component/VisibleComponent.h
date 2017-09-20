#pragma once
#include "../interface/IGameEntity.h"
#include "../data/GraphicData.h"

class VisibleComponent : public BaseComponent
{
public:
	VisibleComponent();
	~VisibleComponent();

	void draw();
	const visiblilityType& getVisiblilityType() const;
	void setVisiblilityType(visiblilityType visiblilityType);
	void addMeshData();
	void addTextureData();
	std::vector<MeshData>& getMeshData();
	std::vector<TextureData>& getTextureData();

private:
	std::vector<MeshData> m_meshData;
	std::vector<TextureData> m_textureData;

	void initialize() override;
	void update() override;
	void shutdown() override;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
};

