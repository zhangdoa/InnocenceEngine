#pragma once

#include <functional>

namespace Inno
{
	class ImGuiWrapper
	{
	public:
		~ImGuiWrapper() {};

		static ImGuiWrapper& Get()
		{
			static ImGuiWrapper instance;
			return instance;
		}
		bool Setup();
		bool Initialize();
		bool Prepare();
		bool ExecuteCommands();
		bool Terminate();

		// Register a callback drawn between ImGui::NewFrame and ImGui::Render.
		// Pointer storage: the caller owns the std::function; it must outlive
		// the wrapper or be removed before destruction. Mirrors the pointer-
		// stable callback contract used by HIDService::AddButtonStateCallback.
		void AddUserDrawCallback(std::function<void()>* in_Callback);

	private:
		ImGuiWrapper() {};
	};
}