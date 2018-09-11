#pragma once
#include "../interface/IPhysicsSystem.h"
#include "../interface/ILogSystem.h"
#include "../interface/IGameSystem.h"
#include "../interface/IAssetSystem.h"
#include "../common/ComponentHeaders.h"

extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;
extern IAssetSystem* g_pAssetSystem;

class PhysicsSystem : public IPhysicsSystem
{
public:
	PhysicsSystem() {};
	~PhysicsSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void setupComponents();
	void setupCameraComponents();
	void setupCameraComponentProjectionMatrix(CameraComponent* cameraComponent);
	void setupCameraComponentRayOfEye(CameraComponent* cameraComponent);
	void setupCameraComponentFrustumVertices(CameraComponent* cameraComponent);
	void setupVisibleComponents();
	void setupLightComponents();
	void setupLightComponentRadius(LightComponent* lightComponent);

	std::vector<Vertex> generateNDC();
	void generateAABB(VisibleComponent & visibleComponent);
	void generateAABB(LightComponent & lightComponent);
	void generateAABB(CameraComponent & cameraComponent);
	AABB generateAABB(const std::vector<Vertex>& vertices);
	AABB generateAABB(const vec4& boundMax, const vec4& boundMin);

	void updateCameraComponents();
	void updateLightComponents();
};
