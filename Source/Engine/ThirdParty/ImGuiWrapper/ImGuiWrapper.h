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
		bool Update();
		bool Render();
		bool Terminate();

	private:
		ImGuiWrapper() {};
	};
}