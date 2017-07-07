#pragma once
#include "IGameEntity.h"

class IVisibleGameEntity : public BaseComponent
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	virtual void render() = 0;
};

