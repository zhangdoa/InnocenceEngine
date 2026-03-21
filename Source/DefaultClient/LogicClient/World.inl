#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/ComponentManager.h"
#include "../../Engine/Services/CameraSystem.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/RayTracer/RayTracer.h"
#include "../../Engine/Component/TransformComponent.h"
#include "../../Engine/Component/LightComponent.h"
#include "../../Engine/Component/CameraComponent.h"

#include "../../Engine/Engine.h"

using namespace Inno;

#include "Player.inl"

namespace Inno
{
	class WorldSystem : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_DEFAULT(WorldSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update();
		bool Terminate() override;
		ObjectStatus GetStatus() { return m_ObjectStatus; }

	private:
		bool setupReferenceSpheres();
		bool setupOcclusionCubes();
		bool setupOpaqueSpheres();
		bool setupTransparentCubes();
		bool setupVolumetricCubes();
		bool setupPointLights();

		//bool updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel = ShaderModel::Opaque);
		void updateSpheres();

		void runTest(uint32_t testTime, std::function<bool()> testCase);

		Vec4 getMousePositionInWorldSpace();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		Player* m_player = nullptr;

		std::vector<EntityID> m_ReferenceSphereEntities;
		std::vector<EntityID> m_OpaqueSphereEntities;
		std::vector<EntityID> m_TransparentCubeEntities;
		std::vector<EntityID> m_VolumetricCubeEntities;
		std::vector<EntityID> m_OcclusionCubeEntities;

		std::vector<EntityID> m_PointLightEntities;

		Vec4 m_posOffset;
		std::default_random_engine m_generator;

		std::function<void()> f_sceneLoadingFinishedCallback;
		std::function<void()> f_loadTestScene;
		std::function<void()> f_convertModel;

		std::function<void()> f_runRayTracing;
		std::function<void()> f_pauseGame;

		float m_seed = 0.0f;
		bool allowUpdate = true;
		uint32_t m_matrixDim = 8;
	};

	bool WorldSystem::setupReferenceSpheres()
	{
		float l_breadthInterval = 4.0f;
		auto l_containerSize = m_matrixDim * m_matrixDim;

		m_ReferenceSphereEntities.clear();
		m_ReferenceSphereEntities.reserve(l_containerSize);

		auto l_Registry = g_Engine->Get<EntityRegistry>();

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_entityName = std::string("MaterialReferenceSphere_" + std::to_string(i) + "/");
			m_ReferenceSphereEntities.emplace_back(l_Registry->Spawn(ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < m_matrixDim; i++)
		{
			for (uint32_t j = 0; j < m_matrixDim; j++)
			{
				auto l_Entity = m_ReferenceSphereEntities[i * m_matrixDim + j];
				auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);
				l_Transform.m_LocalScale = Vec3(1.0f, 1.0f, 1.0f);
				auto l_pos = m_posOffset +
					Vec4(
						(-(m_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval) + 100.0f,
						2.0f,
						(j * l_breadthInterval) - 2.0f * (m_matrixDim - 1),
						0.0f);
				l_Transform.m_LocalPos = Vec3(l_pos.x, l_pos.y, l_pos.z);
				// TODO: load mesh and material via AssetService once AssetService is migrated
				g_Engine->getRenderingServer()->Initialize(l_Entity);
			}
		}

		return true;
	}

	bool WorldSystem::setupOcclusionCubes()
	{
		uint32_t matrixDim = 4;
		float l_breadthInterval = 42.0f;
		auto l_containerSize = matrixDim * matrixDim;

		m_OcclusionCubeEntities.clear();
		m_OcclusionCubeEntities.reserve(l_containerSize);

		auto l_Registry = g_Engine->Get<EntityRegistry>();

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_entityName = std::string("OcclusionCube_" + std::to_string(i) + "/");
			m_OcclusionCubeEntities.emplace_back(l_Registry->Spawn(ObjectLifespan::Scene, l_entityName.c_str()));
		}

		std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);
		std::uniform_real_distribution<float> l_randomHeightDelta(16.0f, 48.0f);
		std::uniform_real_distribution<float> l_randomWidthDelta(4.0f, 6.0f);
		std::uniform_real_distribution<float> l_randomDepthDelta(6.0f, 8.0f);

		auto l_halfMatrixDim = float(matrixDim - 1) / 2.0f;
		auto l_offset = l_halfMatrixDim * l_breadthInterval;

		for (uint32_t i = 0; i < matrixDim; i++)
		{
			for (uint32_t j = 0; j < matrixDim; j++)
			{
				auto l_Entity = m_OcclusionCubeEntities[i * matrixDim + j];
				auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);

				auto l_heightOffset = l_halfMatrixDim * 3.0f - std::abs((float)i - l_halfMatrixDim) - std::abs((float)j - l_halfMatrixDim);
				l_heightOffset *= 4.0f;
				l_Transform.m_LocalScale = Vec3(l_randomWidthDelta(m_generator), l_heightOffset, l_randomDepthDelta(m_generator));

				auto l_pos = m_posOffset +
					Vec4(
						(i * l_breadthInterval) - l_offset,
						l_Transform.m_LocalScale.y / 2.0f,
						(j * l_breadthInterval) - l_offset,
						0.0f);
				l_Transform.m_LocalPos = Vec3(l_pos.x, l_pos.y, l_pos.z);
				l_Transform.m_LocalRot = Math::calcRotatedLocalRotator(l_Transform.m_LocalRot,
					Vec4(0.0f, 1.0f, 0.0f, 0.0f),
					l_randomRotDelta(m_generator));
				// TODO: load mesh and material via AssetService once AssetService is migrated
				g_Engine->getRenderingServer()->Initialize(l_Entity);
			}
		}

		return true;
	}

	bool WorldSystem::setupOpaqueSpheres()
	{
		float l_breadthInterval = 4.0f;
		auto l_containerSize = m_matrixDim * m_matrixDim;

		m_OpaqueSphereEntities.clear();
		m_OpaqueSphereEntities.reserve(l_containerSize);

		auto l_Registry = g_Engine->Get<EntityRegistry>();

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_entityName = std::string("PhysicsTestOpaqueObject_" + std::to_string(i) + "/");
			m_OpaqueSphereEntities.emplace_back(l_Registry->Spawn(ObjectLifespan::Scene, l_entityName.c_str()));
		}

		std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
		std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);

		for (uint32_t i = 0; i < m_matrixDim; i++)
		{
			for (uint32_t j = 0; j < m_matrixDim; j++)
			{
				auto l_Entity = m_OpaqueSphereEntities[i * m_matrixDim + j];
				auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);
				l_Transform.m_LocalScale = Vec3(1.0f, 1.0f, 1.0f);
				auto l_pos = m_posOffset +
					Vec4(
						(-(m_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval),
						l_randomPosDelta(m_generator) * 50.0f,
						(j * l_breadthInterval) - 2.0f * (m_matrixDim - 1),
						0.0f);
				l_Transform.m_LocalPos = Vec3(l_pos.x, l_pos.y, l_pos.z);
				l_Transform.m_LocalRot = Math::calcRotatedLocalRotator(l_Transform.m_LocalRot,
					Vec4(l_randomPosDelta(m_generator), l_randomPosDelta(m_generator), l_randomPosDelta(m_generator), 0.0f).normalize(),
					l_randomRotDelta(m_generator));
				// TODO: load mesh and material via AssetService once AssetService is migrated
				g_Engine->getRenderingServer()->Initialize(l_Entity);
			}
		}

		return true;
	}

	bool WorldSystem::setupTransparentCubes()
	{
		float l_breadthInterval = 4.0f;
		uint32_t l_containerSize = 8;

		m_TransparentCubeEntities.clear();
		m_TransparentCubeEntities.reserve(l_containerSize);

		auto l_Registry = g_Engine->Get<EntityRegistry>();

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_entityName = std::string("PhysicsTestTransparentCube_" + std::to_string(i) + "/");
			m_TransparentCubeEntities.emplace_back(l_Registry->Spawn(ObjectLifespan::Scene, l_entityName.c_str()));
		}

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_Entity = m_TransparentCubeEntities[i];
			auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);
			l_Transform.m_LocalScale = Vec3(1.0f * i, 1.0f * i, 0.5f);
			l_Transform.m_LocalPos = Vec3(0.0f, 2.0f * i, -(i * l_breadthInterval) - 4.0f);
			// TODO: load mesh and material via AssetService once AssetService is migrated
			g_Engine->getRenderingServer()->Initialize(l_Entity);
		}

		return true;
	}

	bool WorldSystem::setupVolumetricCubes()
	{
		uint32_t l_containerSize = 8;

		m_VolumetricCubeEntities.clear();
		m_VolumetricCubeEntities.reserve(l_containerSize);

		auto l_Registry = g_Engine->Get<EntityRegistry>();

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_entityName = std::string("PhysicsTestVolumetricCube_" + std::to_string(i) + "/");
			m_VolumetricCubeEntities.emplace_back(l_Registry->Spawn(ObjectLifespan::Scene, l_entityName.c_str()));
		}

		std::uniform_real_distribution<float> l_randomPosDelta(-40.0f, 40.0f);

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_Entity = m_VolumetricCubeEntities[i];
			auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);
			l_Transform.m_LocalScale = Vec3(4.0f, 4.0f, 4.0f);
			l_Transform.m_LocalPos = Vec3(l_randomPosDelta(m_generator), 2.0f, l_randomPosDelta(m_generator));
			// TODO: load mesh and material via AssetService once AssetService is migrated
			g_Engine->getRenderingServer()->Initialize(l_Entity);
		}

		return true;
	}

	bool WorldSystem::setupPointLights()
	{
		uint32_t l_matrixDim = 16;
		float l_breadthInterval = 4.0f;

		auto l_containerSize = l_matrixDim * l_matrixDim;

		m_PointLightEntities.clear();
		m_PointLightEntities.reserve(l_containerSize);

		std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
		std::uniform_real_distribution<float> l_randomLuminousFlux(10.0f, 100.0f);
		std::uniform_real_distribution<float> l_randomColorTemperature(2000.0f, 14000.0f);

		auto l_Registry = g_Engine->Get<EntityRegistry>();

		for (uint32_t i = 0; i < l_containerSize; i++)
		{
			auto l_entityName = std::string("TestPointLight_" + std::to_string(i) + "/");
			auto l_Entity = l_Registry->Spawn(ObjectLifespan::Scene, l_entityName.c_str());
			m_PointLightEntities.emplace_back(l_Entity);

			auto& l_Light = l_Registry->Emplace<LightComponent>(l_Entity);
			l_Light.m_LightType = LightType::Point;
			l_Light.m_LuminousFlux = l_randomLuminousFlux(m_generator);
			l_Light.m_ColorTemperature = l_randomColorTemperature(m_generator);

			l_Registry->Emplace<TransformComponent>(l_Entity);
		}

		for (uint32_t i = 0; i < l_matrixDim; i++)
		{
			for (uint32_t j = 0; j < l_matrixDim; j++)
			{
				auto* l_Transform = l_Registry->Get<TransformComponent>(m_PointLightEntities[i * l_matrixDim + j]);
				if (l_Transform && m_player && m_player->m_PlayerCameraEntity != INVALID_ENTITY)
				{
					auto* l_CameraTransform = l_Registry->Get<TransformComponent>(m_player->m_PlayerCameraEntity);
					if (l_CameraTransform)
					{
						l_Transform->m_LocalPos = l_CameraTransform->m_LocalPos +
							Vec3(
								(-(l_matrixDim - 1.0f) * l_breadthInterval * l_randomPosDelta(m_generator) / 2.0f) + (i * l_breadthInterval),
								l_randomPosDelta(m_generator) * 32.0f,
								(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1));
					}
				}
			}
		}

		return true;
	}

	bool WorldSystem::Setup(ISystemConfig* systemConfig)
	{
		auto l_testQuatToMat = [&]() -> bool {
			std::uniform_real_distribution<float> randomAxis(0.0f, 1.0f);
			auto axisSample = Vec4(randomAxis(m_generator) * 2.0f - 1.0f, randomAxis(m_generator) * 2.0f - 1.0f, randomAxis(m_generator) * 2.0f - 1.0f, 0.0f);
			axisSample = axisSample.normalize();

			std::uniform_real_distribution<float> randomAngle(0.0f, 360.0f);
			auto angleSample = randomAngle(m_generator);

			Vec4 originalRot = Math::getQuatRotator(axisSample, angleSample);
			Mat4 rotMat = Math::toRotationMatrix(originalRot);
			auto resultRot = Math::toQuatRotator(rotMat);

			auto testResult = true;
			testResult &= (std::abs(std::abs(originalRot.w) - std::abs(resultRot.w)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.x) - std::abs(resultRot.x)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.y) - std::abs(resultRot.y)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.z) - std::abs(resultRot.z)) < epsilon<float, 4>);

			return testResult;
			};

		runTest(512, l_testQuatToMat);

		f_runRayTracing = [&]() { g_Engine->Get<RayTracer>()->Execute(); };
		f_pauseGame = [&]() { allowUpdate = !allowUpdate; };

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_N, true }, ButtonEvent{ EventLifeTime::OneShot, &f_runRayTracing });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_F, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseGame });

		f_sceneLoadingFinishedCallback = [&]() {
			if (!m_player)
				m_player = new Player();
			m_player->Setup();

			// TODO Phase2-migrate: compute posOffset from player camera TransformComponent
			m_posOffset.z -= 75.0f;

			m_posOffset = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

			setupReferenceSpheres();
			//setupOcclusionCubes();
			setupOpaqueSpheres();
			setupTransparentCubes();
			setupVolumetricCubes();
			setupPointLights();

			m_ObjectStatus = ObjectStatus::Activated;
			};

		g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);

		return true;
	}

	bool WorldSystem::Initialize()
	{
		bool l_result = true;

		g_Engine->Get<SceneService>()->Load("..//Res//Scenes//UnitTest.InnoScene");

		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestBox.InnoScene");
		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSibenik.InnoScene");
		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSponza_PBR.InnoScene");
		//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestFireplaceRoom.InnoScene");

		f_loadTestScene = []() {
			//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSponza_PBR.InnoScene");
			//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestSibenik.InnoScene");
			g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestBox.InnoScene");
			//g_Engine->Get<SceneService>()->Load("..//Res//Scenes//GITestFireplaceRoom.InnoScene");
			};

		f_convertModel = []() {
			g_Engine->Get<AssetService>()->Import("..//Res//Models//Sponza_PBR//NewSponza_Merged.fbx");
			};

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_R, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadTestScene });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_Y, true }, ButtonEvent{ EventLifeTime::OneShot, &f_convertModel });

		return true;
	}

	// bool WorldSystem::updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel)
	// {
	// 	if (!model)
	// 		return false;

	// 	for (uint64_t j = 0; j < model->renderableSets.m_count; j++)
	// 	{
	// 		// auto l_pair = g_Engine->Get<AssetService>()->GetRenderableSet(model->renderableSets.m_startOffset + j);
	// 		// l_pair->material->m_materialAttributes.AlbedoR = albedo.x;
	// 		// l_pair->material->m_materialAttributes.AlbedoG = albedo.y;
	// 		// l_pair->material->m_materialAttributes.AlbedoB = albedo.z;
	// 		// l_pair->material->m_materialAttributes.Metallic = MRAT.x;
	// 		// l_pair->material->m_materialAttributes.Roughness = MRAT.y;
	// 		// l_pair->material->m_materialAttributes.AO = MRAT.z;
	// 		// l_pair->material->m_materialAttributes.Alpha = albedo.w;
	// 		// l_pair->material->m_materialAttributes.Thickness = MRAT.w;
	// 		// l_pair->material->m_ShaderModel = shaderModel;
	// 	}
	// 	return true;
	// }

	bool WorldSystem::Update()
	{
		if (m_ObjectStatus != ObjectStatus::Activated)
			return false;

		if (!allowUpdate)
			return false;

		auto l_tickTime = g_Engine->getTickTime();
		m_seed += (l_tickTime / 1000.0f);

		auto l_seed = (1.0f - l_tickTime / 100.0f);
		l_seed = l_seed > 0.0f ? l_seed : 0.01f;
		l_seed = l_seed > 0.85f ? 0.85f : l_seed;

		if (m_player)
			m_player->Update(l_seed);

		updateSpheres();
		return true;
	}

	bool WorldSystem::Terminate()
	{
		if (m_player)
		{
			m_player->Terminate();
			delete m_player;
			return true;
		}

		return false;
	}

	void WorldSystem::runTest(uint32_t testTime, std::function<bool()> testCase)
	{
		Log(Verbose, "Start test...");
		for (uint32_t i = 0; i < testTime; i++)
		{
			auto l_result = testCase();
			if (!l_result)
			{
				Log(Warning, "Test failure.");
			}
		}

		Log(Verbose, "Finished test for ", testTime, " times.");
	}

	Vec4 WorldSystem::getMousePositionInWorldSpace()
	{
		auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		auto l_mousePositionSS = g_Engine->Get<HIDService>()->GetMousePosition();

		auto l_x = 2.0f * l_mousePositionSS.x / l_screenResolution.x - 1.0f;
		auto l_y = 1.0f - 2.0f * l_mousePositionSS.y / l_screenResolution.y;
		auto l_z = -1.0f;
		auto l_w = 1.0f;
		Vec4 l_ndcSpace = Vec4(l_x, l_y, l_z, l_w);

		auto l_activeCamera = g_Engine->Get<CameraSystem>()->GetActiveCamera();
		if (l_activeCamera == nullptr)
		{
			return Vec4();
		}

		auto pCamera = l_activeCamera->m_ProjectionMatrix;
		// TODO Phase2-migrate: need EntityID for active camera to get TransformComponent
		auto rCamera = Mat4();
		auto tCamera = Mat4();

		l_ndcSpace = pCamera.inverse() * l_ndcSpace;
		l_ndcSpace.z = -1.0f;
		l_ndcSpace.w = 0.0f;
		l_ndcSpace = tCamera.inverse() * l_ndcSpace;
		l_ndcSpace = rCamera.inverse() * l_ndcSpace;
		l_ndcSpace = l_ndcSpace.normalize();
		return l_ndcSpace;
	}

	void WorldSystem::updateSpheres()
	{
		// TODO: material updates via EntityRegistry MaterialComponent once mesh/material loading is migrated
	}
}
