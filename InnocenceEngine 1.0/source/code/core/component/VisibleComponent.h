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
	const textureWrapMethod& getTextureWrapMethod() const;
	void setTextureWrapMethod(textureWrapMethod textureWrapMethod);

	void addGraphicData();
	std::vector<GraphicData>& getGraphicData();

private:
	std::vector<GraphicData> m_graphicData;

	void initialize() override;
	void update() override;
	void shutdown() override;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
};

