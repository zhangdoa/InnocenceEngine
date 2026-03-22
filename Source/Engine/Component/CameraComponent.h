#pragma once
#include "../Common/MathHelper.h"
#include "../Interface/ISystem.h"

namespace Inno
{
	struct CameraComponent
	{
		static uint32_t GetTypeID() { return 4; };
		static const char* GetTypeName() { return "CameraComponent"; };

		Mat4 m_ProjectionMatrix = {};
		Frustum m_Frustum = {};
		Ray m_RayOfEye = {};
		float m_FOVX = 90.0f;
		float m_WidthScale = 16.0f;
		float m_HeightScale = 9.0f;
		float m_ZNear = 0.001f;
		float m_ZFar = 1000.0f;
		float m_WHRatio = 16.0f / 9.0f;
		float m_Aperture = 2.2f;
		float m_ShutterTime = 1.0f / 2000.0f;
		float m_ISO = 100.0f;

		std::array<Vertex, 8> m_FrustumVerticesWS = {};
	};

	class ICameraSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ICameraSystem);

		virtual void SetMainCamera(CameraComponent* cameraComponent) = 0;
		virtual CameraComponent* GetMainCamera() = 0;
		virtual void SetActiveCamera(CameraComponent* cameraComponent) = 0;
		virtual CameraComponent* GetActiveCamera() = 0;
	};
}
