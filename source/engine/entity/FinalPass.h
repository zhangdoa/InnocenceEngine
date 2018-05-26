#pragma once
#include "interface/IRenderPass.h"
#include "entity/BaseGraphicPrimitiveHeader.h"
#include "entity/GLGraphicPrimitiveHeader.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;

class FinalPass : public IRenderPass
{
public:
	FinalPass() {};
	~FinalPass() {};

	void setup() override;
	void initialize() override;
	void draw() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	BaseFrameBuffer* m_skyPassFrameBuffer;
	BaseShaderProgram* m_skyPassShaderProgram;
	textureID m_skyPassTextureID;

	BaseFrameBuffer* m_debuggerPassFrameBuffer;
	BaseShaderProgram* m_debuggerPassShaderProgram;
	textureID m_debuggerPassTextureID;

	BaseFrameBuffer* m_billboardPassFrameBuffer;
	BaseShaderProgram* m_billboardPassShaderProgram;
	textureID m_billboardPassTextureID;

	BaseFrameBuffer* m_bloomExtractPassFrameBuffer;
	BaseShaderProgram* m_bloomExtractPassShaderProgram;
	textureID m_bloomExtractPassTextureID;

	BaseFrameBuffer* m_bloomBlurPassPingFrameBuffer;
	BaseFrameBuffer* m_bloomBlurPassPongFrameBuffer;
	BaseShaderProgram* m_bloomBlurPassShaderProgram;
	textureID m_bloomBlurPassPingTextureID;
	textureID m_bloomBlurPassPongTextureID;

	BaseFrameBuffer* m_finalPassFrameBuffer;
	BaseShaderProgram* m_finalPassShaderProgram;
	textureID m_finalPassTextureID;

};
