#pragma once
#include "IGameEntity.h"

class IVisibleGameEntity : public IGameEntity
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	virtual void render() = 0;
};

