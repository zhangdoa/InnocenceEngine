#pragma once
#include "../Interface/ISystem.h"
#include "../Component/CameraComponent.h"

namespace Inno
{
	class CameraSystem : public ICameraSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(CameraSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void SetMainCamera(CameraComponent* cameraComponent) override;
		CameraComponent* GetMainCamera() override;
		void SetActiveCamera(CameraComponent* cameraComponent) override;
		CameraComponent* GetActiveCamera() override;
	};
}