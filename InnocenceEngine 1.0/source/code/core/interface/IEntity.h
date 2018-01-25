#pragma once

#include "IObject.h"
#include "../manager/MemoryManager.h"

#ifndef _I_ENTITY_H_
#define _I_ENTITY_H_

typedef unsigned long int EntityID;

class IEntity : public IObject
{
public:
	IEntity();
	virtual ~IEntity();

	virtual void setup() override;
	const EntityID& getEntityID() const;
	const std::string& getClassName() const;

private:
	EntityID m_entityID = 0;
	std::string m_className = {};
};

#endif // !_I_ENTITY_H_