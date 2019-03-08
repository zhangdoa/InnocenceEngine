#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"

INNO_INTERFACE IRenderingFrontendSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingFrontendSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual bool anyUninitializedMeshDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual bool anyUninitializedTextureDataComponent() = 0;

	INNO_SYSTEM_EXPORT virtual void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) = 0;
	INNO_SYSTEM_EXPORT virtual void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) = 0;

	INNO_SYSTEM_EXPORT virtual MeshDataComponent* acquireUninitializedMeshDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* acquireUninitializedTextureDataComponent() = 0;
};