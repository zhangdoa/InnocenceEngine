#pragma once
#include "IGameEntity.h"

enum class visibleGameEntityType { INVISIBLE, STATIC_MESH, SKYBOX };

class IVisibleGameEntity : public BaseComponent
{
public:
	IVisibleGameEntity();
	virtual ~IVisibleGameEntity();

	virtual void render() = 0;

	const visibleGameEntityType& getVisibleGameEntityType() const;
	void setVisibleGameEntityType(visibleGameEntityType visibleGameEntityType);

private:
	visibleGameEntityType m_visibleGameEntityType = visibleGameEntityType::INVISIBLE;
};

