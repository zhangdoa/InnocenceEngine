#pragma once
#include "../interface/IGameEntity.h"
#include "../data/GraphicData.h"

class VisibleComponent : public BaseComponent
{
public:
	VisibleComponent();
	~VisibleComponent();

	void initialize() override;
	void update() override;
	void shutdown() override;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;

	graphicDataMap& getGraphicDataMap();
	void setGraphicDataMap(graphicDataMap& graphicDataMap);
	void addMeshData(meshDataID& meshDataID);
	void addTextureData(textureDataPair& textureDataPair);
	void overwriteTextureData(textureDataPair& textureDataPair);

private:
	graphicDataMap m_graphicDataMap;
};

