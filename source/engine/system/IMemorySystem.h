#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

#include "../common/ComponentHeaders.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"

#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLFrameBufferComponent.h"
#include "../component/GLShaderProgramComponent.h"
#include "../component/GLRenderPassComponent.h"

#if defined INNO_PLATFORM_WIN
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"
#include "../component/DXShaderProgramComponent.h"
#include "../component/DXRenderPassComponent.h"
#endif

#include "../component/VKMeshDataComponent.h"
#include "../component/VKTextureDataComponent.h"
#include "../component/VKShaderProgramComponent.h"
#include "../component/VKRenderPassComponent.h"

#include "../component/PhysicsDataComponent.h"

#define allocateComponentInterfaceDecl( className ) \
virtual className* allocate##className() = 0;

#define spawnComponentTemplate( className ) \
inline className* IMemorySystem::spawn() \
{ \
	auto t = allocate##className(); \
	return t; \
};

#define freeComponentInterfaceDecl( className ) \
virtual bool free##className(className* p) = 0;

#define destroyComponentTemplate( className ) \
inline bool IMemorySystem::destroy(className* p) \
{ \
	reinterpret_cast<className*>(p)->~className(); \
	return free##className(p); \
};

INNO_INTERFACE IMemorySystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IMemorySystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

protected:
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(TransformComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DirectionalLightComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(PointLightComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(SphereLightComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(CameraComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(InputComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(EnvironmentCaptureComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(MeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(MaterialDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(TextureDataComponent);

	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(GLMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(GLTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(GLFrameBufferComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(GLShaderProgramComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(GLRenderPassComponent);

	#if defined INNO_PLATFORM_WIN
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXShaderProgramComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXRenderPassComponent);
	#endif

	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(VKMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(VKTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(VKShaderProgramComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(VKRenderPassComponent);

	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(PhysicsDataComponent);

	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(TransformComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DirectionalLightComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(PointLightComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(SphereLightComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(CameraComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(InputComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(EnvironmentCaptureComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(MeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(MaterialDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(TextureDataComponent);

	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(GLMeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(GLTextureDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(GLFrameBufferComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(GLShaderProgramComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(GLRenderPassComponent);

	#if defined INNO_PLATFORM_WIN
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DXTextureDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DXShaderProgramComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DXRenderPassComponent);
	#endif

	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(VKMeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(VKTextureDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(VKShaderProgramComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(VKRenderPassComponent);

	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(PhysicsDataComponent);

public:
	template <typename T> T * spawn()
	{
		return nullptr;
	};

	template <typename T> bool destroy(T* p)
	{
		reinterpret_cast<T*>(p)->~T();
		return free(p);
	};

	INNO_SYSTEM_EXPORT virtual void* allocateRawMemory(size_t size) = 0;
};

template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(TransformComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(VisibleComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DirectionalLightComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(PointLightComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(SphereLightComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(CameraComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(InputComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(EnvironmentCaptureComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(MeshDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(MaterialDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(TextureDataComponent);

template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(GLMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(GLTextureDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(GLFrameBufferComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(GLShaderProgramComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(GLRenderPassComponent);

#if defined INNO_PLATFORM_WIN
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DXMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DXTextureDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DXShaderProgramComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DXRenderPassComponent);
#endif

template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(VKMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(VKTextureDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(VKShaderProgramComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(VKRenderPassComponent);

template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(PhysicsDataComponent);

template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(TransformComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(VisibleComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DirectionalLightComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(PointLightComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(SphereLightComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(CameraComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(InputComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(EnvironmentCaptureComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(MeshDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(MaterialDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(TextureDataComponent);

template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(GLMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(GLTextureDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(GLFrameBufferComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(GLShaderProgramComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(GLRenderPassComponent);

#if defined INNO_PLATFORM_WIN
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DXMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DXTextureDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DXShaderProgramComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DXRenderPassComponent);
#endif

template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(VKMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(VKTextureDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(VKShaderProgramComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(VKRenderPassComponent);

template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(PhysicsDataComponent);