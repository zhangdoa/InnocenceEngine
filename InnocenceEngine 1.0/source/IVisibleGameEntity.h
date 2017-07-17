#pragma once
#include "IGameEntity.h"

class IVisibleGameEntity : public BaseComponent
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	enum visibleGameEntityType { INVISIBLE, STATIC_MESH, SKYBOX };

	const int getVisibleGameEntityType() const;
	void setVisibleGameEntityType(visibleGameEntityType visibleGameEntityType);

private:
	visibleGameEntityType m_visibleGameEntityType = INVISIBLE;
};

