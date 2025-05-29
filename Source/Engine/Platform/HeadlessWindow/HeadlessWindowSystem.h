#pragma once
#include "../../Interface/IWindowSystem.h"
#include "../../Common/Object.h"

namespace Inno
{
	class IWindowSurface;
	
	class HeadlessWindowSystem : public IWindowSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(HeadlessWindowSystem);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;
		std::vector<std::type_index> GetDependencies() override;

		IWindowSurface* GetWindowSurface() override;
		bool SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam) override;
		void ConsumeEvents(const WindowEventProcessCallback& p_Callback) override;
		bool AddEventCallback(WindowEventCallback* callback) override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		IWindowSurface* m_dummySurface = nullptr;
	};
}
