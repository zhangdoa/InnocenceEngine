#pragma once
#include "IGameSystem.h"

#define registerComponentImplDecl( className ) \
INNO_SYSTEM_EXPORT void registerComponent(className* rhs, const EntityID& parentEntity) override;

#define getComponentImplDecl( className ) \
INNO_SYSTEM_EXPORT className* get##className(const EntityID& parentEntity) override;

class InnoGameSystem : INNO_IMPLEMENT IGameSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGameSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	registerComponentImplDecl(TransformComponent);
	registerComponentImplDecl(VisibleComponent);
	registerComponentImplDecl(DirectionalLightComponent);
	registerComponentImplDecl(PointLightComponent);
	registerComponentImplDecl(SphereLightComponent);
	registerComponentImplDecl(CameraComponent);
	registerComponentImplDecl(InputComponent);
	registerComponentImplDecl(EnvironmentCaptureComponent);
	
	getComponentImplDecl(TransformComponent);
	getComponentImplDecl(VisibleComponent);
	getComponentImplDecl(DirectionalLightComponent);
	getComponentImplDecl(PointLightComponent);
	getComponentImplDecl(SphereLightComponent);
	getComponentImplDecl(CameraComponent);
	getComponentImplDecl(InputComponent);
	getComponentImplDecl(EnvironmentCaptureComponent);

	INNO_SYSTEM_EXPORT std::string getGameName() override;

	INNO_SYSTEM_EXPORT TransformComponent* getRootTransformComponent() override;

	INNO_SYSTEM_EXPORT void registerButtonStatusCallback(InputComponent* inputComponent, ButtonData boundButton, std::function<void()>* function) override;
	INNO_SYSTEM_EXPORT void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) override;
	
	INNO_SYSTEM_EXPORT void saveComponentsCapture() override;
	INNO_SYSTEM_EXPORT void setGameInstance(IGameInstance* rhs) override;

	INNO_SYSTEM_EXPORT EntityID createEntity(const std::string& entityName) override;
	INNO_SYSTEM_EXPORT bool removeEntity(const std::string& entityName) override;
	INNO_SYSTEM_EXPORT std::string getEntityName(const EntityID & entityID) override;
	INNO_SYSTEM_EXPORT EntityID getEntityID(const std::string & entityName) override;

};
