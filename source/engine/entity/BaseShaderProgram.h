#pragma once
#include "interface/IObject.hpp"
#include "innoMath.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/ILogSystem.h"
#include "ComponentHeaders.h"
#include "BaseMesh.h"
#include "BaseTexture.h"

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;

class BaseShaderProgram : public IObject
{
public:
	BaseShaderProgram() {};
	virtual ~BaseShaderProgram() {};

	void setup() override;
	void setup(const std::vector<shaderData>& shaderDatas);
	void update() override;
	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap, shaderDrawPair in_shaderDrawPair = shaderDrawPair(shaderDrawPolygonType::FILL, shaderDrawTextureType::FULL)) = 0;
	const objectStatus& getStatus() const override;

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	std::vector<shaderData> m_shaderDatas;
};
