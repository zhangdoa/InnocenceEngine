#pragma once
#include "../Common/InnoObject.h"
#include "../Common/InnoMathHelper.h"
#include "../Interface/ISystem.h"

namespace Inno
{
	class CameraComponent : public InnoComponent
	{
	public:		
		static uint32_t GetTypeID() { return 4; };
		static const char* GetTypeName() { return "CameraComponent"; };
		
		Mat4 m_projectionMatrix;
		Frustum m_frustum;
		Ray m_rayOfEye;
		float m_FOVX = 0.0f;
		float m_widthScale = 0.0f;
		float m_heightScale = 0.0f;
		float m_zNear = 0.0f;
		float m_zFar = 0.0f;
		float m_WHRatio = 0.0f;
		float m_aperture = 2.2f;
		float m_shutterTime = 1.0f / 2000.0f;
		float m_ISO = 100.0f;

		std::vector<Vertex> m_splitFrustumVerticesWS;
	};

	class ICameraSystem : public IComponentSystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ICameraSystem);

		virtual void SetMainCamera(CameraComponent* cameraComponent) = 0;
		virtual CameraComponent* GetMainCamera() = 0;
		virtual void SetActiveCamera(CameraComponent* cameraComponent) = 0;
		virtual CameraComponent* GetActiveCamera() = 0;
	};
}