#include "BaseTexture.h"

void BaseTexture::setup()
{
	g_pLogSystem->printLog("BaseTexture: Warning: use the setup() with parameter!");
}


void BaseTexture::setup(textureType textureType, textureColorComponentsFormat textureColorComponentsFormat, texturePixelDataFormat texturePixelDataFormat, textureWrapMethod textureWrapMethod, textureFilterMethod textureMinFilterMethod, textureFilterMethod textureMagFilterMethod, int textureWidth, int textureHeight, texturePixelDataType texturePixelDataType, const std::vector<void*>& textureData)
{
	m_textureType = textureType;
	m_textureColorComponentsFormat = textureColorComponentsFormat;
	m_texturePixelDataFormat = texturePixelDataFormat;
	m_textureWrapMethod = textureWrapMethod;
	m_textureMinFilterMethod = textureMinFilterMethod;
	m_textureMagFilterMethod = textureMagFilterMethod;
	m_textureWidth = textureWidth;
	m_textureHeight = textureHeight;
	m_texturePixelDataType = texturePixelDataType;
	m_textureData = textureData;
}

const objectStatus & BaseTexture::getStatus() const
{
	return m_objectStatus;
}

const textureID BaseTexture::getTextureID() const
{
	return m_textureID;
}

const int BaseTexture::getTextureWidth() const
{
	return m_textureWidth;
}

const int BaseTexture::getTextureHeight() const
{
	return m_textureHeight;
}