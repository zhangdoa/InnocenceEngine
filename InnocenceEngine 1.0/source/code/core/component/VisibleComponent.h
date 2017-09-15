#pragma once
#include "../interface/IGameEntity.h"
#include "../data/GraphicData.h"

enum class visiblilityType { INVISIBLE, STATIC_MESH, SKYBOX };

class VisibleComponent : public BaseComponent
{
public:
	VisibleComponent();
	~VisibleComponent();

	const visiblilityType& getVisiblilityType() const;
	void setVisiblilityType(visiblilityType visiblilityType);
	std::vector<StaticMeshData>&getMeshData() const;
	std::vector<TextureData>&getTextureData() const;
	void addMeshData();
	void addTextureData();

private:
	std::vector<StaticMeshData> m_meshData;
	std::vector<TextureData> m_textureData;

	void initialize() override;
	void update() override;
	void shutdown() override;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
};

