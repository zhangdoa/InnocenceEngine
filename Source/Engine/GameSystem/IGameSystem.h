#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"
#include "../Common/ComponentHeaders.h"

#define getComponentContainerInterfaceCall( className ) \
get##className##s()

INNO_INTERFACE IGameSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual const EntityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() = 0;
	virtual std::string getGameName() = 0;
};