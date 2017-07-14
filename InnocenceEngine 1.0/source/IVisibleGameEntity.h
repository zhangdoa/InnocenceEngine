#pragma once
#include "IGameEntity.h"

class IVisibleGameEntity : public BaseComponent
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	enum visibleGameEntityType { STATIC_MESH, SKYBOX};

	virtual void render() = 0;
};

