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

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"
#endif

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
virtual void free##className(className* p) = 0;

#define destroyComponentTemplate( className ) \
inline void IMemorySystem::destroy(className* p) \
{ \
	reinterpret_cast<className*>(p)->~className(); \
	free##className(p); \
};

#define getClassNameTemplate( className ) \
inline std::string IMemorySystem::getClassName<className>() \
{ \
	return std::string(#className); \
}

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
	INNO_SYSTEM_EXPORT virtual void* allocate(unsigned long size) = 0;
	INNO_SYSTEM_EXPORT virtual void free(void* ptr) = 0;
	INNO_SYSTEM_EXPORT virtual void serializeImpl(const std::string& fileName, const std::string& className, unsigned long classSize, void* ptr) = 0;
	INNO_SYSTEM_EXPORT virtual void* deserializeImpl(const std::string& fileName) = 0;

	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(TransformComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(LightComponent);
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
	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXTextureDataComponent);
	#endif
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(PhysicsDataComponent);

INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(TransformComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(LightComponent);
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
	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(DXTextureDataComponent);
	#endif
	INNO_SYSTEM_EXPORT freeComponentInterfaceDecl(PhysicsDataComponent);
public:
	INNO_SYSTEM_EXPORT virtual void dumpToFile(bool fullDump) = 0;

	template <typename T> T * spawn()
	{
		return reinterpret_cast<T *>(allocate(sizeof(T)));
	};

	template <typename T>  T * spawn(size_t n)
	{
		return reinterpret_cast<T *>(allocate(n * sizeof(T)));
	};

	template <typename T> void destroy(T* p)
	{
		reinterpret_cast<T*>(p)->~T();
		free(p);
	};

	template <typename T> void serialize(const std::string& fileName, T* p)
	{
		auto className = getClassName<T>();
		serializeImpl(fileName, className, sizeof(T), p);
	};

	template <typename T> T* deserialize(const std::string& fileName)
	{
		return reinterpret_cast<T *>(deserializeImpl(fileName));
	};

	template<typename T> std::string getClassName()
	{
		return std::string("Undefined");
	}
};

template<>
getClassNameTemplate(TransformComponent);

template<>
getClassNameTemplate(VisibleComponent);

template<>
getClassNameTemplate(LightComponent);

template<>
getClassNameTemplate(CameraComponent);

template<>
getClassNameTemplate(InputComponent);

template<>
getClassNameTemplate(EnvironmentCaptureComponent);

template<>
getClassNameTemplate(MeshDataComponent);

template<>
getClassNameTemplate(MaterialDataComponent);

template<>
getClassNameTemplate(TextureDataComponent);

template<>
getClassNameTemplate(PhysicsDataComponent);

template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(TransformComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(VisibleComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(LightComponent);
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
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DXMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(DXTextureDataComponent);
#endif
template <>
INNO_SYSTEM_EXPORT spawnComponentTemplate(PhysicsDataComponent);

template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(TransformComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(VisibleComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(LightComponent);
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
#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DXMeshDataComponent);
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(DXTextureDataComponent);
#endif
template <>
INNO_SYSTEM_EXPORT destroyComponentTemplate(PhysicsDataComponent);