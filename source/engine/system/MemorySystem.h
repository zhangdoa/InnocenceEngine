#pragma once
#include "IMemorySystem.h"

#define allocateComponentImplDecl( className ) \
className* allocate##className() override;

class InnoMemorySystem : INNO_IMPLEMENT IMemorySystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoMemorySystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	INNO_SYSTEM_EXPORT void* allocate(unsigned long size) override;
	INNO_SYSTEM_EXPORT void free(void* ptr) override;
	INNO_SYSTEM_EXPORT void serializeImpl(void* ptr) override;
	INNO_SYSTEM_EXPORT void* deserializeImpl(unsigned long size, const std::string& filePath) override;

	INNO_SYSTEM_EXPORT void dumpToFile(bool fullDump) override;

	INNO_SYSTEM_EXPORT allocateComponentImplDecl(TransformComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(VisibleComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(LightComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(CameraComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(InputComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(EnvironmentCaptureComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(MeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(TextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLTextureDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLFrameBufferComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(GLShaderProgramComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DXMeshDataComponent);
	INNO_SYSTEM_EXPORT allocateComponentImplDecl(DXTextureDataComponent);
};
