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

	void addGraphicData();
	void addMeshData(GameObjectID meshDataIndex);
	std::vector<GameObjectID>& getMeshData();
	void addTextureData(GameObjectID textureDataIndex);
	std::vector<GameObjectID>& getTextureData();

private:
	std::vector<std::tuple<GameObjectID, std::list<GameObjectID>>> m_graphicDatas;
	std::vector<GameObjectID> m_meshDatas;
	std::vector<GameObjectID> m_textureDatas;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

