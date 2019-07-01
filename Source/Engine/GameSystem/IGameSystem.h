#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"
#include "../Common/ComponentHeaders.h"

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
	spawnComponentInterfaceDecl(CameraComponent);

	registerComponentInterfaceDecl(CameraComponent);

	destroyComponentInterfaceDecl(CameraComponent);

	unregisterComponentInterfaceDecl(CameraComponent);

	getComponentInterfaceDecl(CameraComponent);

	getComponentContainerInterfaceDecl(CameraComponent);

public:
	template <typename T> T * spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
	{
		static_assert(0, "InnoCompileError: Fall back to generalized implementation.");
		return nullptr;
	};

	template <typename T> bool destroy(T* rhs)
	{
		static_assert(0, "InnoCompileError: Fall back to generalized implementation.");
		return false;
	};

	template <typename T> T* get(const InnoEntity* parentEntity)
	{
		static_assert(0, "InnoCompileError: Fall back to generalized implementation.");
		return nullptr;
	};

	template <typename T> std::vector<T*>& get()
	{
		static_assert(0, "InnoCompileError: Fall back to generalized implementation.");
		return nullptr;
	};

	virtual const EntityChildrenComponentsMetadataMap& getEntityChildrenComponentsMetadataMap() = 0;
	virtual std::string getGameName() = 0;
};

template <> inline CameraComponent * IGameSystem::spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage)
{
	return spawnComponentInterfaceCall(CameraComponent, parentEntity, objectSource, objectUsage);
};

template <> inline bool IGameSystem::destroy(CameraComponent* rhs)
{
	return destroyComponentInterfaceCall(rhs);
};

template <> inline CameraComponent * IGameSystem::get(const InnoEntity* parentEntity)
{
	return getComponentInterfaceCall(CameraComponent, parentEntity);
};

template <> inline std::vector<CameraComponent*>& IGameSystem::get()
{
	return getComponentContainerInterfaceCall(CameraComponent);
};