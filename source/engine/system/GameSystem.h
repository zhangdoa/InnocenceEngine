#pragma once
#include "IGameSystem.h"

#define spawnComponentImplDecl( className ) \
className* spawn##className(const EntityID& parentEntity) override;

#define registerComponentImplDecl( className ) \
void registerComponent(className* rhs, const EntityID& parentEntity) override;

#define destroyComponentImplDecl( className ) \
bool destroy(className* rhs) override;

#define unregisterComponentImplDecl( className ) \
void unregisterComponent(className* rhs) override;

#define getComponentImplDecl( className ) \
className* get##className(const EntityID& parentEntity) override;

#define getComponentContainerImplDecl( className ) \
std::vector<className*>& get##className##s() override;

class InnoGameSystem : INNO_IMPLEMENT IGameSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGameSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	spawnComponentImplDecl(TransformComponent);
	spawnComponentImplDecl(VisibleComponent);
	spawnComponentImplDecl(DirectionalLightComponent);
	spawnComponentImplDecl(PointLightComponent);
	spawnComponentImplDecl(SphereLightComponent);
	spawnComponentImplDecl(CameraComponent);
	spawnComponentImplDecl(InputComponent);
	spawnComponentImplDecl(EnvironmentCaptureComponent);

	registerComponentImplDecl(TransformComponent);
	registerComponentImplDecl(VisibleComponent);
	registerComponentImplDecl(DirectionalLightComponent);
	registerComponentImplDecl(PointLightComponent);
	registerComponentImplDecl(SphereLightComponent);
	registerComponentImplDecl(CameraComponent);
	registerComponentImplDecl(InputComponent);
	registerComponentImplDecl(EnvironmentCaptureComponent);

	destroyComponentImplDecl(TransformComponent);
	destroyComponentImplDecl(VisibleComponent);
	destroyComponentImplDecl(DirectionalLightComponent);
	destroyComponentImplDecl(PointLightComponent);
	destroyComponentImplDecl(SphereLightComponent);
	destroyComponentImplDecl(CameraComponent);
	destroyComponentImplDecl(InputComponent);
	destroyComponentImplDecl(EnvironmentCaptureComponent);

	unregisterComponentImplDecl(TransformComponent);
	unregisterComponentImplDecl(VisibleComponent);
	unregisterComponentImplDecl(DirectionalLightComponent);
	unregisterComponentImplDecl(PointLightComponent);
	unregisterComponentImplDecl(SphereLightComponent);
	unregisterComponentImplDecl(CameraComponent);
	unregisterComponentImplDecl(InputComponent);
	unregisterComponentImplDecl(EnvironmentCaptureComponent);

	getComponentImplDecl(TransformComponent);
	getComponentImplDecl(VisibleComponent);
	getComponentImplDecl(DirectionalLightComponent);
	getComponentImplDecl(PointLightComponent);
	getComponentImplDecl(SphereLightComponent);
	getComponentImplDecl(CameraComponent);
	getComponentImplDecl(InputComponent);
	getComponentImplDecl(EnvironmentCaptureComponent);

	getComponentContainerImplDecl(TransformComponent);
	getComponentContainerImplDecl(VisibleComponent);
	getComponentContainerImplDecl(DirectionalLightComponent);
	getComponentContainerImplDecl(PointLightComponent);
	getComponentContainerImplDecl(SphereLightComponent);
	getComponentContainerImplDecl(CameraComponent);
	getComponentContainerImplDecl(InputComponent);
	getComponentContainerImplDecl(EnvironmentCaptureComponent);

	std::string getGameName() override;
	TransformComponent* getRootTransformComponent() override;
	EntityNameMap& getEntityNameMap() override;
	EntityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() override;

	void registerButtonStatusCallback(InputComponent* inputComponent, ButtonData boundButton, std::function<void()>* function) override;
	void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) override;

	void saveComponentsCapture() override;
	void cleanScene() override;

	EntityID createEntity(const std::string& entityName) override;
	bool removeEntity(const std::string& entityName) override;
	std::string getEntityName(const EntityID & entityID) override;
	EntityID getEntityID(const std::string & entityName) override;
};
