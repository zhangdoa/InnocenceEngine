#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"
#include "IMemorySystem.h"

#define spawnComponentInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual void spawnComponent(className* rhs, EntityID parentEntity) = 0;

#define getComponentInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual className* get##className(EntityID parentEntity) = 0;

#define getComponentInterfaceCall( className, parentEntity ) \
get##className(parentEntity)

INNO_INTERFACE IGameSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	spawnComponentInterfaceDecl(TransformComponent);
	spawnComponentInterfaceDecl(VisibleComponent);
	spawnComponentInterfaceDecl(LightComponent);
	spawnComponentInterfaceDecl(CameraComponent);
	spawnComponentInterfaceDecl(InputComponent);
	spawnComponentInterfaceDecl(EnvironmentCaptureComponent);

	spawnComponentInterfaceDecl(MaterialDataComponent);

	template <typename T> T * spawn(EntityID parentEntity)
	{
		auto l_ptr = g_pMemorySystem->spawn<T>();
		if (l_ptr)
		{
			spawnComponent(l_ptr, parentEntity);
			return l_ptr;
		}
		else
		{
			return nullptr;
		}
	};

	getComponentInterfaceDecl(TransformComponent);
	getComponentInterfaceDecl(VisibleComponent);
	getComponentInterfaceDecl(LightComponent);
	getComponentInterfaceDecl(CameraComponent);
	getComponentInterfaceDecl(InputComponent);
	getComponentInterfaceDecl(EnvironmentCaptureComponent);

	getComponentInterfaceDecl(MaterialDataComponent);

	template <typename T> T * get(EntityID parentEntity)
	{
		return new T();
	};

	INNO_SYSTEM_EXPORT virtual std::string getGameName() = 0;
	INNO_SYSTEM_EXPORT virtual TransformComponent* getRootTransformComponent() = 0;

	INNO_SYSTEM_EXPORT virtual void registerButtonStatusCallback(InputComponent* inputComponent, ButtonData boundButton, std::function<void()>* function) = 0;
	INNO_SYSTEM_EXPORT virtual void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) = 0;

	INNO_SYSTEM_EXPORT virtual void saveComponentsCapture() = 0;

	IMemorySystem* g_pMemorySystem;
};

template <> inline TransformComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(TransformComponent, parentEntity);
};

template <> inline VisibleComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(VisibleComponent, parentEntity);
};

template <> inline LightComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(LightComponent, parentEntity);
};

template <> inline CameraComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(CameraComponent, parentEntity);
};

template <> inline InputComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(InputComponent, parentEntity);
};

template <> inline EnvironmentCaptureComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(EnvironmentCaptureComponent, parentEntity);
};

template <> inline MaterialDataComponent * IGameSystem::get(EntityID parentEntity)
{
	return getComponentInterfaceCall(MaterialDataComponent, parentEntity);
};
