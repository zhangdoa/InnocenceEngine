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

	const visiblilityType& getVisiblilityType() const;
	void setVisiblilityType(visiblilityType visiblilityType);

	const meshDrawMethod& getMeshDrawMethod() const;
	void setMeshDrawMethod(meshDrawMethod meshDrawMethod);

	const textureWrapMethod& getTextureWrapMethod() const;
	void setTextureWrapMethod(textureWrapMethod textureWrapMethod);

	graphicDataMap& getGraphicDataMap();
	void setGraphicDataMap(graphicDataMap& graphicDataMap);
	void addMeshData(meshDataID meshDataID);
	void addTextureData(meshDataID meshDataID, textureDataID textureDataID, textureType textureType);
	void addTextureData(textureDataID textureDataID, textureType textureType);


private:
	graphicDataMap m_graphicDataMap;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

