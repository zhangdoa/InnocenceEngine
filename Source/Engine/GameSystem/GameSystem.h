#pragma once
#include "IGameSystem.h"

#define spawnComponentImplDecl( className ) \
className* spawn##className(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;

#define registerComponentImplDecl( className ) \
void registerComponent(className* rhs, const InnoEntity* parentEntity) override;

#define destroyComponentImplDecl( className ) \
bool destroy(className* rhs) override;

#define unregisterComponentImplDecl( className ) \
void unregisterComponent(className* rhs) override;

#define getComponentImplDecl( className ) \
className* get##className(const InnoEntity* parentEntity) override;

#define getComponentContainerImplDecl( className ) \
std::vector<className*>& get##className##s() override;

INNO_CONCRETE InnoGameSystem : INNO_IMPLEMENT IGameSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGameSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	spawnComponentImplDecl(CameraComponent);

	registerComponentImplDecl(CameraComponent);

	destroyComponentImplDecl(CameraComponent);

	unregisterComponentImplDecl(CameraComponent);

	getComponentImplDecl(CameraComponent);

	getComponentContainerImplDecl(CameraComponent);

	std::string getGameName() override;
	const EntityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() override;
};
