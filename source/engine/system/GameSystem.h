#pragma once
#include "IGameSystem.h"

#define spawnComponentImplDecl( className ) \
INNO_SYSTEM_EXPORT void spawnComponent(className* rhs, EntityID parentEntity) override;

#define getComponentImplDecl( className ) \
INNO_SYSTEM_EXPORT className* get##className##(EntityID parentEntity) override;

class InnoGameSystem : INNO_IMPLEMENT IGameSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoGameSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	spawnComponentImplDecl(TransformComponent);
	spawnComponentImplDecl(VisibleComponent);
	spawnComponentImplDecl(LightComponent);
	spawnComponentImplDecl(CameraComponent);
	spawnComponentImplDecl(InputComponent);
	spawnComponentImplDecl(EnvironmentCaptureComponent);

	getComponentImplDecl(TransformComponent);
	getComponentImplDecl(VisibleComponent);
	getComponentImplDecl(LightComponent);
	getComponentImplDecl(CameraComponent);
	getComponentImplDecl(InputComponent);
	getComponentImplDecl(EnvironmentCaptureComponent);

	INNO_SYSTEM_EXPORT std::string getGameName() override;
	INNO_SYSTEM_EXPORT TransformComponent* getRootTransformComponent() override;

	INNO_SYSTEM_EXPORT void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function) override;
	INNO_SYSTEM_EXPORT void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) override;
	
	INNO_SYSTEM_EXPORT void saveComponentsCapture() override;
};
