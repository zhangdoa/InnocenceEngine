#pragma once
#include "interface/IRenderPass.h"
#include "entity/BaseGraphicPrimitiveHeader.h"
#include "entity/GLGraphicPrimitiveHeader.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IAssetSystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;
extern IAssetSystem* g_pAssetSystem;

class ShadowPass : public IRenderPass
{
public:
	ShadowPass() {};
	~ShadowPass() {};

	void setup() override;
	void initialize() override;
	void draw() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	BaseFrameBuffer* m_shadowForwardPassFrameBuffer;
	BaseShaderProgram* m_shadowForwardPassShaderProgram;
	textureID m_shadowForwardPassTextureID_L0;
	textureID m_shadowForwardPassTextureID_L1;
	textureID m_shadowForwardPassTextureID_L2;
	textureID m_shadowForwardPassTextureID_L3;
};
