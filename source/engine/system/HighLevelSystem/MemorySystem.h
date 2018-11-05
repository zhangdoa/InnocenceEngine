#pragma once
#include "../../common/InnoType.h"
#include "../../exports/LowLevelSystem_Export.h"
#include "../../common/ComponentHeaders.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"

#include "../../component/GLMeshDataComponent.h"
#include "../../component/GLTextureDataComponent.h"
#include "../../component/GLFrameBufferComponent.h"
#include "../../component/GLShaderProgramComponent.h"

#include "../../component/DXMeshDataComponent.h"
#include "../../component/DXTextureDataComponent.h"

namespace InnoMemorySystem
{
	InnoLowLevelSystem_EXPORT bool setup();
	InnoLowLevelSystem_EXPORT bool initialize();
	InnoLowLevelSystem_EXPORT bool update();
	InnoLowLevelSystem_EXPORT bool terminate();

	InnoLowLevelSystem_EXPORT void* allocate(unsigned long size);
	InnoLowLevelSystem_EXPORT void free(void* ptr);
	void serializeImpl(void* ptr);
	void* deserializeImpl(unsigned long size, const std::string& filePath);

	InnoLowLevelSystem_EXPORT void dumpToFile(bool fullDump);

	InnoLowLevelSystem_EXPORT TransformComponent* allocateTransformComponent();
	InnoLowLevelSystem_EXPORT VisibleComponent* allocateVisibleComponent();
	InnoLowLevelSystem_EXPORT LightComponent* allocateLightComponent();
	InnoLowLevelSystem_EXPORT CameraComponent* allocateCameraComponent();
	InnoLowLevelSystem_EXPORT InputComponent* allocateInputComponent();
	InnoLowLevelSystem_EXPORT EnvironmentCaptureComponent* allocateEnvironmentCaptureComponent();

	InnoLowLevelSystem_EXPORT MeshDataComponent* allocateMeshDataComponent();
	InnoLowLevelSystem_EXPORT TextureDataComponent* allocateTextureDataComponent();

	InnoLowLevelSystem_EXPORT GLMeshDataComponent* allocateGLMeshDataComponent();
	InnoLowLevelSystem_EXPORT GLTextureDataComponent* allocateGLTextureDataComponent();
	InnoLowLevelSystem_EXPORT GLFrameBufferComponent* allocateGLFrameBufferComponent();
	InnoLowLevelSystem_EXPORT GLShaderProgramComponent* allocateGLShaderProgramComponent();

	InnoLowLevelSystem_EXPORT DXMeshDataComponent* allocateDXMeshDataComponent();
	InnoLowLevelSystem_EXPORT DXTextureDataComponent* allocateDXTextureDataComponent();

	template <typename T> T * spawn()
	{
		return reinterpret_cast<T *>(allocate(sizeof(T)));
	};

	template <>  TransformComponent * spawn()
	{
		auto t = allocateTransformComponent();
		return t;
	};

	template <>  VisibleComponent * spawn()
	{
		auto t = allocateVisibleComponent();
		return t;
	};

	template <>  LightComponent * spawn()
	{
		auto t = allocateLightComponent();
		return t;
	};

	template <>  CameraComponent * spawn()
	{
		auto t = allocateCameraComponent();
		return t;
	};

	template <>  InputComponent * spawn()
	{
		auto t = allocateInputComponent();
		return t;
	};

	template <>  EnvironmentCaptureComponent * spawn()
	{
		auto t = allocateEnvironmentCaptureComponent();
		return t;
	};

	template <>  MeshDataComponent * spawn()
	{
		auto t = allocateMeshDataComponent();
		return t;
	};

	template <>  TextureDataComponent * spawn()
	{
		auto t = allocateTextureDataComponent();
		return t;
	};

	template <>  GLMeshDataComponent * spawn()
	{
		auto t = allocateGLMeshDataComponent();
		return t;
	};

	template <>  GLTextureDataComponent * spawn()
	{
		auto t = allocateGLTextureDataComponent();
		return t;
	};

	template <>  GLFrameBufferComponent * spawn()
	{
		auto t = allocateGLFrameBufferComponent();
		return t;
	};

	template <>  GLShaderProgramComponent * spawn()
	{
		auto t = allocateGLShaderProgramComponent();
		return t;
	};

	template <>  DXMeshDataComponent * spawn()
	{
		auto t = allocateDXMeshDataComponent();
		return t;
	};

	template <>  DXTextureDataComponent * spawn()
	{
		auto t = allocateDXTextureDataComponent();
		return t;
	};

	template <typename T>  T * spawn(size_t n)
	{
		return reinterpret_cast<T *>(allocate(n * sizeof(T)));
	};

	template <typename T> void destroy(T *p)
	{
		reinterpret_cast<T *>(p)->~T();
		InnoMemorySystem::free(p);
	};

	template <typename T> void serialize(T* p)
	{
		serializeImpl(p);
	};

	template <typename T> T* deserialize(const std::string& filePath)
	{
		return reinterpret_cast<T *>(deserializeImpl(sizeof(T), filePath));
	};

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};
