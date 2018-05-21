#pragma once
#include "interface/IGraphicPrimitive.h"
#include "InnoMath.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;

class BaseTexture : public IGraphicPrimitive
{
public:
	BaseTexture() { m_textureID = std::rand(); };
	virtual ~BaseTexture() {};

	void setup() override;
	void setup(textureType textureType, textureColorComponentsFormat textureColorComponentsFormat, texturePixelDataFormat texturePixelDataFormat, textureWrapMethod textureWrapMethod, textureFilterMethod textureMinFilterMethod, textureFilterMethod textureMagFilterMethod, int textureWidth, int textureHeight, texturePixelDataType texturePixelDataType, const std::vector<void *>& textureData);
	virtual void update(int textureIndex) = 0;
	virtual void attachToFramebuffer(int colorAttachmentIndex, int textureIndex, int mipLevel) = 0;
	const objectStatus& getStatus() const override;
	const textureID getTextureID() const;
	const int getTextureWidth() const;
	const int getTextureHeight() const;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	textureID m_textureID;
	textureType m_textureType;
	textureColorComponentsFormat m_textureColorComponentsFormat;
	texturePixelDataFormat m_texturePixelDataFormat;
	textureFilterMethod m_textureMinFilterMethod;
	textureFilterMethod m_textureMagFilterMethod;
	textureWrapMethod m_textureWrapMethod;
	int m_textureWidth;
	int m_textureHeight;
	texturePixelDataType m_texturePixelDataType;
	std::vector<void *> m_textureData;
};
