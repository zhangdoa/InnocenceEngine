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

#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"

#define allocateComponentInterfaceDecl( className ) \
virtual className* allocate##className() = 0;

#define allocateComponentTemplate( className ) \
className* spawn() \
{ \
	auto t = allocate##className(); \
	return t; \
};

INNO_INTERFACE IMemorySystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IMemorySystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual objectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual void* allocate(unsigned long size) = 0;
	INNO_SYSTEM_EXPORT virtual void free(void* ptr) = 0;
	INNO_SYSTEM_EXPORT virtual void serializeImpl(void* ptr) = 0;
	INNO_SYSTEM_EXPORT virtual void* deserializeImpl(unsigned long size, const std::string& filePath) = 0;

	INNO_SYSTEM_EXPORT virtual void dumpToFile(bool fullDump) = 0;

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
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(DXTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(Vertex);
	INNO_SYSTEM_EXPORT allocateComponentInterfaceDecl(Index);

	template <typename T> T * spawn()
	{
		return reinterpret_cast<T *>(allocate(sizeof(T)));
	};

	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(TransformComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(VisibleComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(LightComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(CameraComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(InputComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(EnvironmentCaptureComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(MeshDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(MaterialDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(TextureDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(GLMeshDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(GLTextureDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(GLFrameBufferComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(GLShaderProgramComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(GLRenderPassComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(DXMeshDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(DXTextureDataComponent);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(Vertex);
	template <>
	INNO_SYSTEM_EXPORT allocateComponentTemplate(Index);

	template <typename T>  T * spawn(size_t n)
	{
		return reinterpret_cast<T *>(allocate(n * sizeof(T)));
	};

	template <typename T> void destroy(T *p)
	{
		reinterpret_cast<T *>(p)->~T();
		free(p);
	};

	template <typename T> void serialize(T* p)
	{
		serializeImpl(p);
	};

	template <typename T> T* deserialize(const std::string& filePath)
	{
		return reinterpret_cast<T *>(deserializeImpl(sizeof(T), filePath));
	};
};