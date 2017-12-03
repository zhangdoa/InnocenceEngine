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

	const meshDrawMethod& getMeshDrawMethod() const;
	void setMeshDrawMethod(meshDrawMethod meshDrawMethod);

	const textureWrapMethod& getTextureWrapMethod() const;
	void setTextureWrapMethod(textureWrapMethod textureWrapMethod);

	void addMeshData(MeshData* meshData);
	void addTextureData(TextureData* textureData);

private:
	std::vector<MeshData*> m_meshDatas;
	std::vector<TextureData*> m_textureDatas;

	void initialize() override;
	void update() override;
	void shutdown() override;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

