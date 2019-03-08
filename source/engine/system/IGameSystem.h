#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"
#include "IMemorySystem.h"
#include "../../game/IGameInstance.h"

#define registerComponentInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual void registerComponent(className* rhs, const EntityID& parentEntity) = 0;

#define getComponentInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual className* get##className(const EntityID& parentEntity) = 0;

#define getComponentInterfaceCall( className, parentEntity ) \
get##className(parentEntity)

#define getComponentContainerInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual std::vector<className*>& get##className##s() = 0;

#define getComponentContainerInterfaceCall( className ) \
get##className##s()

INNO_INTERFACE IGameSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

protected:
	registerComponentInterfaceDecl(TransformComponent);
	registerComponentInterfaceDecl(VisibleComponent);
	registerComponentInterfaceDecl(DirectionalLightComponent);
	registerComponentInterfaceDecl(PointLightComponent);
	registerComponentInterfaceDecl(SphereLightComponent);
	registerComponentInterfaceDecl(CameraComponent);
	registerComponentInterfaceDecl(InputComponent);
	registerComponentInterfaceDecl(EnvironmentCaptureComponent);

public:
	template <typename T> T * spawn(const EntityID& parentEntity)
	{
		auto l_ptr = g_pMemorySystem->spawn<T>();
		if (l_ptr)
		{
			registerComponent(l_ptr, parentEntity);
			return l_ptr;
		}
		else
		{
			return nullptr;
		}
	};

	template <typename T> bool destroy(T* rhs)
	{
		return g_pMemorySystem->destroy<T>(rhs);
	};

protected:
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
	template <typename T> T* get(const EntityID& parentEntity)
	{
		return nullptr;
	};

	template <typename T> std::vector<T*>& get()
	{
		return nullptr;
	};

	INNO_SYSTEM_EXPORT virtual entityNameMap& getEntityNameMap() = 0;
	INNO_SYSTEM_EXPORT virtual entityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() = 0;

	INNO_SYSTEM_EXPORT virtual std::string getGameName() = 0;
	INNO_SYSTEM_EXPORT virtual TransformComponent* getRootTransformComponent() = 0;

	INNO_SYSTEM_EXPORT virtual void registerButtonStatusCallback(InputComponent* inputComponent, ButtonData boundButton, std::function<void()>* function) = 0;
	INNO_SYSTEM_EXPORT virtual void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) = 0;

	INNO_SYSTEM_EXPORT virtual void saveComponentsCapture() = 0;
	INNO_SYSTEM_EXPORT virtual void cleanScene() = 0;

	INNO_SYSTEM_EXPORT virtual void pauseGameUpdate(bool shouldPause) = 0;

	INNO_SYSTEM_EXPORT virtual void setGameInstance(IGameInstance* rhs) = 0;

	INNO_SYSTEM_EXPORT virtual EntityID createEntity(const std::string& entityName) = 0;
	INNO_SYSTEM_EXPORT virtual bool removeEntity(const std::string& entityName) = 0;
	INNO_SYSTEM_EXPORT virtual std::string getEntityName(const EntityID & entityID) = 0;
	INNO_SYSTEM_EXPORT virtual EntityID getEntityID(const std::string & entityName) = 0;

	IMemorySystem* g_pMemorySystem;
};

template <> inline TransformComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(TransformComponent, parentEntity);
};

template <> inline VisibleComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(VisibleComponent, parentEntity);
};

template <> inline DirectionalLightComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(DirectionalLightComponent, parentEntity);
};

template <> inline PointLightComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(PointLightComponent, parentEntity);
};

template <> inline SphereLightComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(SphereLightComponent, parentEntity);
};

template <> inline CameraComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(CameraComponent, parentEntity);
};

template <> inline InputComponent * IGameSystem::get(const EntityID& parentEntity)
{
	return getComponentInterfaceCall(InputComponent, parentEntity);
};

template <> inline EnvironmentCaptureComponent * IGameSystem::get(const EntityID& parentEntity)
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