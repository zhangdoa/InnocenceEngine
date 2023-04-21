#pragma once
#include "../Interface/IFileSystem.h"

namespace Inno
{
	class FileSystem : public IFileSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(FileSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		std::string getWorkingDirectory() override;

		std::vector<char> loadFile(const char* filePath, IOMode openMode) override;
		bool saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode) override;

		bool addCPPClassFiles(const CPPClassDesc& desc) override;
	};
}