#pragma once
#include "../common/InnoType.h"
#include "GLFrameBufferComponent.h"
#include "GLShaderProgramComponent.h"
#include "TextureDataComponent.h"
#include "GLTextureDataComponent.h"

class GLEnvironmentRenderPassSingletonComponent
{
public:
	~GLEnvironmentRenderPassSingletonComponent() {};
	
	static GLEnvironmentRenderPassSingletonComponent& getInstance()
	{
		static GLEnvironmentRenderPassSingletonComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFrameBufferComponent* m_BRDFLUTPassFBC;

	GLShaderProgramComponent* m_BRDFSplitSumLUTPassSPC;
	TextureDataComponent* m_BRDFSplitSumLUTPassTDC;
	GLTextureDataComponent* m_BRDFSplitSumLUTPassGLTDC;

	GLShaderProgramComponent* m_BRDFMSAverageLUTPassSPC;
	TextureDataComponent* m_BRDFMSAverageLUTPassTDC;
	GLTextureDataComponent* m_BRDFMSAverageLUTPassGLTDC;
	GLuint m_BRDFMSAverageLUTPass_uni_brdfLUT;
private:
	GLEnvironmentRenderPassSingletonComponent() {};
};
