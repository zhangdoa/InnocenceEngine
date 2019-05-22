#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"

#define spawnComponentInterfaceDecl( className ) \
virtual className* spawn##className(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) = 0;

#define spawnComponentInterfaceCall( className, parentEntity, objectSource, objectUsage ) \
spawn##className(parentEntity, objectSource, objectUsage)

#define registerComponentInterfaceDecl( className ) \
virtual void registerComponent(className* rhs, const InnoEntity* parentEntity) = 0;

#define destroyComponentInterfaceDecl( className ) \
virtual bool destroy(className* rhs) = 0;

#define destroyComponentInterfaceCall( ptr ) \
destroy(ptr)

#define unregisterComponentInterfaceDecl( className ) \
virtual void unregisterComponent(className* rhs) = 0;

#define getComponentInterfaceDecl( className ) \
virtual className* get##className(const InnoEntity* parentEntity) = 0;

#define getComponentInterfaceCall( className, parentEntity ) \
get##className(parentEntity)

#define getComponentContainerInterfaceDecl( className ) \
virtual std::vector<className*>& get##className##s() = 0;

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

protected:
	spawnComponentInterfaceDecl(TransformComponent);
	spawnComponentInterfaceDecl(VisibleComponent);
	spawnComponentInterfaceDecl(DirectionalLightComponent);
	spawnComponentInterfaceDecl(PointLightComponent);
	spawnComponentInterfaceDecl(SphereLightComponent);
	spawnComponentInterfaceDecl(CameraComponent);
	spawnComponentInterfaceDecl(InputComponent);
	spawnComponentInterfaceDecl(EnvironmentCaptureComponent);

	registerComponentInterfaceDecl(TransformComponent);
	registerComponentInterfaceDecl(VisibleComponent);
	registerComponentInterfaceDecl(DirectionalLightComponent);
	registerComponentInterfaceDecl(PointLightComponent);
	registerComponentInterfaceDecl(SphereLightComponent);
	registerComponentInterfaceDecl(CameraComponent);
	registerComponentInterfaceDecl(InputComponent);
	registerComponentInterfaceDecl(EnvironmentCaptureComponent);

	destroyComponentInterfaceDecl(TransformComponent);
	destroyComponentInterfaceDecl(VisibleComponent);
	destroyComponentInterfaceDecl(DirectionalLightComponent);
	destroyComponentInterfaceDecl(PointLightComponent);
	destroyComponentInterfaceDecl(SphereLightComponent);
	destroyComponentInterfaceDecl(CameraComponent);
	destroyComponentInterfaceDecl(InputComponent);
	destroyComponentInterfaceDecl(EnvironmentCaptureComponent);

	unregisterComponentInterfaceDecl(TransformComponent);
	unregisterComponentInterfaceDecl(VisibleComponent);
	unregisterComponentInterfaceDecl(DirectionalLightComponent);
	unregisterComponentInterfaceDecl(PointLightComponent);
	unregisterComponentInterfaceDecl(SphereLightComponent);
	unregisterComponentInterfaceDecl(CameraComponent);
	unregisterComponentInterfaceDecl(InputComponent);
	unregisterComponentInterfaceDecl(EnvironmentCaptureComponent);

	getComponentInterfaceDecl(TransformComponent);
	getComponentInterfaceDecl(VisibleComponent);
	getComponentInterfaceDecl(DirectionalLightComponent);
	getComponentInterfaceDecl(PointLightComponent);
	getComponentInterfaceDecl(SphereLightComponent);
	getComponentInterfaceDecl(CameraComponent);
	getComponentInterfaceDecl(InputComponent);
	getComponentInterfaceDecl(EnvironmentCaptureComponent);

	getComponentContainerInterfaceDecl(TransformComponent);
	getComponentContainerInterfaceDecl(VisibleComponent);
	getComponentContainerInterfaceDecl(DirectionalLightComponent);
	getComponentContainerInterfaceDecl(PointLightComponent);
	getComponentContainerInterfaceDecl(SphereLightComponent);
	getComponentContainerInterfaceDecl(CameraComponent);
	getComponentContainerInterfaceDecl(InputComponent);
	getComponentContainerInterfaceDecl(EnvironmentCaptureComponent);

public:
	template <typename T> T * spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
	{
		return nullptr;
	};

	template <typename T> bool destroy(T* rhs)
	{
		return false;
	};

	template <typename T> T* get(const InnoEntity* parentEntity)
	{
		return nullptr;
	};

	template <typename T> std::vector<T*>& get()
	{
		return nullptr;
	};

	virtual const std::vector<InnoEntity*>& getEntities() = 0;
	virtual const EntityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() = 0;

	virtual std::string getGameName() = 0;
	virtual TransformComponent* getRootTransformComponent() = 0;

	virtual void registerButtonStatusCallback(InputComponent* inputComponent, ButtonData boundButton, std::function<void()>* function) = 0;
	virtual void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) = 0;

	virtual void saveComponentsCapture() = 0;

	virtual InnoEntity* createEntity(const EntityName& entityName, ObjectSource objectSource, ObjectUsage objectUsage) = 0;
	virtual bool removeEntity(const InnoEntity* entity) = 0;
	virtual InnoEntity* getEntity(const EntityName& entityName) = 0;
};

template <> inline TransformComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(TransformComponent, parentEntity, objectSource, objectUsage);
};

template <> inline VisibleComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(VisibleComponent, parentEntity, objectSource, objectUsage);
};

template <> inline DirectionalLightComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(DirectionalLightComponent, parentEntity, objectSource, objectUsage);
};

template <> inline PointLightComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(PointLightComponent, parentEntity, objectSource, objectUsage);
};

template <> inline SphereLightComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(SphereLightComponent, parentEntity, objectSource, objectUsage);
};

template <> inline CameraComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(CameraComponent, parentEntity, objectSource, objectUsage);
};

template <> inline InputComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(InputComponent, parentEntity, objectSource, objectUsage);
};

template <> inline EnvironmentCaptureComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(EnvironmentCaptureComponent, parentEntity, objectSource, objectUsage);
};

template <> inline bool IGameSystem::destroy(TransformComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(VisibleComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(DirectionalLightComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(PointLightComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(SphereLightComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(CameraComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(InputComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline bool IGameSystem::destroy(EnvironmentCaptureComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline TransformComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(TransformComponent, parentEntity);
};

template <> inline VisibleComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(VisibleComponent, parentEntity);
};

template <> inline DirectionalLightComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(DirectionalLightComponent, parentEntity);
};

template <> inline PointLightComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(PointLightComponent, parentEntity);
};

template <> inline SphereLightComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(SphereLightComponent, parentEntity);
};

template <> inline CameraComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(CameraComponent, parentEntity);
};

template <> inline InputComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(InputComponent, parentEntity);
};

template <> inline EnvironmentCaptureComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(EnvironmentCaptureComponent, parentEntity);
};

template <> inline std::vector<TransformComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(TransformComponent);
};

template <> inline std::vector<VisibleComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(VisibleComponent);
};

template <> inline std::vector<DirectionalLightComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(DirectionalLightComponent);
};

template <> inline std::vector<PointLightComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(PointLightComponent);
};

template <> inline std::vector<SphereLightComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(SphereLightComponent);
};

template <> inline std::vector<CameraComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(CameraComponent);
};

template <> inline std::vector<InputComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(InputComponent);
};

template <> inline std::vector<EnvironmentCaptureComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(EnvironmentCaptureComponent);
};
