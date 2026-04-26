#pragma once
#include "../Common/MathHelper.h"
#include "../Interface/IService.h"

namespace Inno
{
	enum class ExposureMode : uint32_t
	{
		Manual = 0,
		Auto = 1,
	};

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

		// Exposure-mode toggle and auto-exposure controls. Defaults preserve
		// TASK-142 behavior: Auto with K=6.0 (canonical AgX-Default mid-grey
		// target) and zero EV bias. Manual mode falls back to the
		// aperture/shutter/ISO triple above.
		ExposureMode m_ExposureMode = ExposureMode::Auto;
		float m_AutoExposureKey = 6.0f;
		float m_AutoExposureCompensation = 0.0f;

		std::array<Vertex, 8> m_FrustumVerticesWS = {};
	};

	class ICameraService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ICameraService);

		virtual void SetMainCamera(CameraComponent* cameraComponent) = 0;
		virtual CameraComponent* GetMainCamera() = 0;
		virtual void SetActiveCamera(CameraComponent* cameraComponent) = 0;
		virtual CameraComponent* GetActiveCamera() = 0;
	};
}
