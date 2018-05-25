#pragma once
#include "interface/IRenderPass.h"
#include "entity/BaseGraphicPrimitiveHeader.h"
#include "entity/GLGraphicPrimitiveHeader.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;

class LightPass : public IRenderPass
{
public:
	LightPass() {};
	~LightPass() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	BaseFrameBuffer* m_lightPassFrameBuffer;
	BaseShaderProgram* m_lightPassShaderProgram;
	textureID m_lightPassTextureID;
};
