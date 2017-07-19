#pragma once
#include "IGameEntity.h"

class IVisibleGameEntity : public BaseComponent
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	enum visibleGameEntityType { INVISIBLE, STATIC_MESH, SKYBOX };

	virtual void render() = 0;

	const int getVisibleGameEntityType() const;
	void setVisibleGameEntityType(visibleGameEntityType visibleGameEntityType);

private:
	visibleGameEntityType m_visibleGameEntityType = INVISIBLE;
};

