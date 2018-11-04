#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IRenderingSystem.h"
#include "DXHeaders.h"
#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DXMeshDataComponent.h"
#include "../../component/DXTextureDataComponent.h"

class DXRenderingSystem : INNO_IMPLEMENT IRenderingSystem
{
public:
	InnoHighLevelSystem_EXPORT bool setup() override;
	InnoHighLevelSystem_EXPORT bool initialize() override;
	InnoHighLevelSystem_EXPORT bool update() override;
	InnoHighLevelSystem_EXPORT bool terminate() override;

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
