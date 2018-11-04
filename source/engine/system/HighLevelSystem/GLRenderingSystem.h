#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IRenderingSystem.h"
#include "../../component/GLRenderingSystemSingletonComponent.h"
#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/GLMeshDataComponent.h"
#include "../../component/GLTextureDataComponent.h"
#include "../../component/GLFrameBufferComponent.h"
#include "../../component/GLShaderProgramComponent.h"
#include "../LowLevelSystem/MemorySystem.h"

class GLRenderingSystem : INNO_IMPLEMENT IRenderingSystem
{
public:
	InnoHighLevelSystem_EXPORT bool setup() override;
	InnoHighLevelSystem_EXPORT bool initialize() override;
	InnoHighLevelSystem_EXPORT bool update() override;
	InnoHighLevelSystem_EXPORT bool terminate() override;

	InnoHighLevelSystem_EXPORT objectStatus getStatus() override;
};