#pragma once
#include "interface/IGraphicPrimitive.h"
#include "InnoMath.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/ILogSystem.h"
#include "ComponentHeaders.h"
#include "BaseMesh.h"
#include "BaseTexture.h"

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;

class BaseShaderProgram : public IGraphicPrimitive
{
public:
	BaseShaderProgram() {};
	virtual ~BaseShaderProgram() {};

	void setup() override;
	void setup(const std::vector<shaderData>& shaderDatas);
	const objectStatus& getStatus() const override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::vector<shaderData> m_shaderDatas;
};
