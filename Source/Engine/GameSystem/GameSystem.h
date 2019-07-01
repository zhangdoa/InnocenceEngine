#pragma once
#include "IGameSystem.h"

INNO_CONCRETE InnoGameSystem : INNO_IMPLEMENT IGameSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGameSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	std::string getGameName() override;
	const EntityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() override;
};
