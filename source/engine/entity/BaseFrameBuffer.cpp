#include "BaseFrameBuffer.h"

void BaseFrameBuffer::setup()
{
	g_pLogSystem->printLog("BaseFrameBuffer: Warning: use the setup() with parameter!");
}

void BaseFrameBuffer::setup(frameBufferType frameBufferType, renderBufferType renderBufferType, const std::vector<vec2>& renderBufferStorageSize, const std::vector<BaseTexture*>& renderTargetTextures, const std::vector<BaseShaderProgram*>& shaderPrograms)
{
	m_frameBufferType = frameBufferType;
	m_renderBufferType = renderBufferType;
	m_renderBufferStorageSize = renderBufferStorageSize;
	m_renderTargetTextures = renderTargetTextures;
	m_shaderPrograms = shaderPrograms;
}

void BaseFrameBuffer::update()
{
	g_pLogSystem->printLog("BaseFrameBuffer: Warning: use the update() with parameter!");
}

const unsigned int BaseFrameBuffer::getRenderTargetNumber() const
{
	return m_renderTargetTextures.size();
}

const objectStatus & BaseFrameBuffer::getStatus() const
{
	return m_objectStatus;
}