#pragma once
#include "IGameEntity.h"
#include "../manager/RenderingManager.h"

enum class visibleGameEntityType { INVISIBLE, STATIC_MESH, SKYBOX };

class IVisibleGameEntity : public BaseComponent
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	const visibleGameEntityType& getVisibleGameEntityType() const;
	void setVisibleGameEntityType(visibleGameEntityType visibleGameEntityType);

private:
	visibleGameEntityType m_visibleGameEntityType = visibleGameEntityType::INVISIBLE;
};

