#pragma once
#include "interface/IRenderPass.h"
#include "entity/BaseGraphicPrimitiveHeader.h"
#include "entity/GLGraphicPrimitiveHeader.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;

class GeometryPass : public IRenderPass
{
public:
	GeometryPass() {};
	~GeometryPass() {};

	void setup() override;
	void initialize() override;
	void draw() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	BaseFrameBuffer* m_geometryPassFrameBuffer;
	BaseShaderProgram* m_geometryPassShaderProgram;
	textureID m_geometryPassRT0TextureID;
	textureID m_geometryPassRT1TextureID;
	textureID m_geometryPassRT2TextureID;
	textureID m_geometryPassRT3TextureID;
};
