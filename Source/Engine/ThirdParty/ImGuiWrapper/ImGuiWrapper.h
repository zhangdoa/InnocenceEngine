#pragma once

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

	private:
		ImGuiWrapper() {};
	};
}