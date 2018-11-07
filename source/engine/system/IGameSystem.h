#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../common/ComponentHeaders.h"
#include "IMemorySystem.h"

#define spawnComponentInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual void spawnComponent(className* rhs) = 0;

#define getComponentInterfaceDecl( className ) \
INNO_SYSTEM_EXPORT virtual className* get##className##(EntityID parentEntity) = 0;

#define getComponentInterfaceCall( className, parentEntity ) \
get##className##(parentEntity)

INNO_INTERFACE IGameSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual objectStatus getStatus() = 0;

	spawnComponentInterfaceDecl(TransformComponent);
	spawnComponentInterfaceDecl(VisibleComponent);
	spawnComponentInterfaceDecl(LightComponent);
	spawnComponentInterfaceDecl(CameraComponent);
	spawnComponentInterfaceDecl(InputComponent);
	spawnComponentInterfaceDecl(EnvironmentCaptureComponent);

	template <typename T> T * spawn()
	{
		auto l_ptr = g_pMemorySystem->spawn<T>();
		if (l_ptr)
		{
			spawnComponent(l_ptr);
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

	template <typename T> T * get(EntityID parentEntity)
	{
			return getComponentInterfaceCall(T, parentEntity);
	};

	virtual std::string getGameName() = 0;

	virtual	void registerButtonStatusCallback(InputComponent* inputComponent, button boundButton, std::function<void()>* function) = 0;
	virtual void registerMouseMovementCallback(InputComponent* inputComponent, int mouseCode, std::function<void(float)>* function) = 0;

	virtual void saveComponentsCapture() = 0;

	IMemorySystem* g_pMemorySystem;
};
