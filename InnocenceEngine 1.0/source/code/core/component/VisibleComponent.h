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

	void addMeshData(meshDataID meshDataID);
	void addTextureData(meshDataID meshDataID, textureDataID textureDataID, textureType textureType);
	graphicDataMap& getGraphicDatas();

private:
	graphicDataMap m_graphicDatas;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

