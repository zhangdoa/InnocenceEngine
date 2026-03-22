#pragma once
#include "../Interface/IService.h"
#include "../Component/CameraComponent.h"

namespace Inno
{
	class CameraService : public ICameraService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(CameraService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
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