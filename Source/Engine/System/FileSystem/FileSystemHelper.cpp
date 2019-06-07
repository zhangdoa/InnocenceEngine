#include "FileSystemHelper.h"

#include "../../Common/STL17.h"
#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "../ICoreSystem.h"
extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	std::string m_workingDir;
}

bool InnoFileSystemNS::setup()
{
#if defined INNO_PLATFORM_WIN
	m_workingDir = fs::current_path().parent_path().generic_string();
#else
	m_workingDir = fs::current_path().generic_string();
#endif
	m_workingDir = m_workingDir + "//";

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: current working directory is " + m_workingDir);

	return true;
}

std::string InnoFileSystemNS::loadTextFile(const std::string & fileName)
{
	std::ifstream file;

	file.open((m_workingDir + fileName).c_str());

	if (!file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open text file : " + fileName + "!");
		return std::string();
	}

	std::stringstream ss;
	std::string output;

	ss << file.rdbuf();
	output = ss.str();
	file.close();

	return output;
}

std::vector<char> InnoFileSystemNS::loadBinaryFile(const std::string & fileName)
{
	std::ifstream file;
	file.open((m_workingDir + fileName).c_str(), std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: can't open binary file : " + fileName + "!");
		return std::vector<char>();
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

bool InnoFileSystemNS::isFileExist(const std::string & filePath)
{
	if (fs::exists(fs::path(m_workingDir + filePath)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string InnoFileSystemNS::getFileExtension(const std::string & filePath)
{
	return fs::path(filePath).extension().generic_string();
}

std::string InnoFileSystemNS::getFileName(const std::string & filePath)
{
	return fs::path(filePath).stem().generic_string();
}

std::string InnoFileSystemNS::getWorkingDirectory()
{
	return m_workingDir;
}
