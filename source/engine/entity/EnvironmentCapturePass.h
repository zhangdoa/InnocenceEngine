#pragma once
#include "interface/IRenderPass.h"
#include "entity/BaseGraphicPrimitiveHeader.h"
#include "entity/GLGraphicPrimitiveHeader.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;

class EnvironmentCapturePass : public IRenderPass
{
public:
	EnvironmentCapturePass() {};
	~EnvironmentCapturePass() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	BaseFrameBuffer * m_environmentPassFrameBuffer;
	BaseShaderProgram* m_environmentCapturePassShaderProgram;
	BaseShaderProgram* m_environmentConvolutionPassShaderProgram;
	BaseShaderProgram* m_environmentPreFilterPassShaderProgram;
	BaseShaderProgram* m_environmentBRDFLUTPassShaderProgram;
	textureID m_environmentCapturePassTextureID;
	textureID m_environmentConvolutionPassTextureID;
	textureID m_environmentPreFilterPassTextureID;
	textureID m_environmentBRDFLUTTextureID;
};
