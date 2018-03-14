#pragma once

#include "interface/IRenderingSystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IGameSystem.h"
#include "interface/ILogSystem.h"

#include "GLWindowSystem.h"
#include "GLInputSystem.h"
#include "GLRenderingSystem.h"

#include "entity/ComponentHeaders.h"

extern ILogSystem* g_pLogSystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

class RenderingSystem : public IRenderingSystem
{
public:
	RenderingSystem() {};
	~RenderingSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void render() override;
	meshID addMesh() override;
	textureID addTexture(textureType textureType) override;
	BaseMesh* getMesh(meshID meshID) override;
	BaseTexture* getTexture(textureType textureType, textureID textureID) override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void assignUnitMesh(VisibleComponent& visibleComponent, meshType meshType);
	void assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void loadTexture(const std::vector<std::string>& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void assignloadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);

	std::vector<std::unique_ptr<ISystem>> m_childSystem;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;

	meshID m_UnitCubeTemplate;
	meshID m_UnitSphereTemplate;
	meshID m_UnitQuadTemplate;
	textureID m_basicNormalTemplate;
	textureID m_basicAlbedoTemplate;
	textureID m_basicMetallicTemplate;
	textureID m_basicRoughnessTemplate;
	textureID m_basicAOTemplate;

	std::unordered_map<std::string, modelMap> m_loadedModelMap;
	std::unordered_map<std::string, texturePair> m_loadedTextureMap;
};

