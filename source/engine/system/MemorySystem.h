#pragma once
#include "IMemorySystem.h"

#define allocateComponentImplDecl( className ) \
className* allocate##className() override;

#define freeComponentImplDecl( className ) \
void free##className(className* p) override;

class InnoMemorySystem : INNO_IMPLEMENT IMemorySystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoMemorySystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT void* allocate(unsigned long size) override;
	INNO_SYSTEM_EXPORT void free(void* ptr) override;
	INNO_SYSTEM_EXPORT void serializeImpl(const std::string& fileName, const std::string& className, unsigned long classSize, void* ptr) override;
	INNO_SYSTEM_EXPORT void* deserializeImpl(const std::string& fileName) override;

	INNO_SYSTEM_EXPORT void dumpToFile(bool fullDump) override;

	INNO_SYSTEM_EXPORT allocateComponentImplDecl(TransformComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DirectionalLightComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(PointLightComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(SphereLightComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(CameraComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(InputComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(EnvironmentCaptureComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(MeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(MaterialDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(TextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLFrameBufferComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLShaderProgramComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLRenderPassComponent);
	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DXTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DXShaderProgramComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DXRenderPassComponent);
	#endif
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(PhysicsDataComponent);

	INNO_SYSTEM_EXPORT freeComponentImplDecl(TransformComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(DirectionalLightComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(PointLightComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(SphereLightComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(CameraComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(InputComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(EnvironmentCaptureComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(MeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(MaterialDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(TextureDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(GLMeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(GLTextureDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(GLFrameBufferComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(GLShaderProgramComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(GLRenderPassComponent);
	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	INNO_SYSTEM_EXPORT freeComponentImplDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(DXTextureDataComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(DXShaderProgramComponent);
	INNO_SYSTEM_EXPORT freeComponentImplDecl(DXRenderPassComponent);
	#endif
	INNO_SYSTEM_EXPORT freeComponentImplDecl(PhysicsDataComponent);
};
