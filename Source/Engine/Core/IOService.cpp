#include "IOService.h"

#include "../Common/STL17.h"
#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "InnoLogger.h"

namespace IOService
{
	std::string m_workingDir;
}

bool IOService::setupWorkingDirectory()
{
#if defined INNO_PLATFORM_WIN || defined INNO_PLATFORM_LINUX
	m_workingDir = fs::current_path().parent_path().generic_string();
#else
	m_workingDir = fs::current_path().generic_string();
#endif
	m_workingDir = m_workingDir + "//";

	InnoLogger::Log(LogLevel::Verbose, "IOService: current working directory is ", m_workingDir.c_str());

	return true;
}

std::vector<char> IOService::loadFile(const char* filePath, IOMode openMode)
{
	std::ios_base::openmode l_mode = std::ios::in;
	switch (openMode)
	{
	case IOMode::Text:
		l_mode = std::ios::in;
		break;
	case IOMode::Binary:
		l_mode = std::ios::in | std::ios::ate | std::ios::binary;
		break;
	default:
		break;
	}

	std::ifstream l_file;

	l_file.open((m_workingDir + filePath).c_str(), l_mode);

	if (!l_file.is_open())
	{
		InnoLogger::Log(LogLevel::Error, "IOService: can't open file : ", filePath, "!");
		return std::vector<char>();
	}

	auto pbuf = l_file.rdbuf();
	std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
	pbuf->pubseekpos(0, l_file.in);

	std::vector<char> buffer(l_size);
	pbuf->sgetn(&buffer[0], l_size);

	l_file.close();

	return buffer;
}

bool IOService::saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode)
{
	std::ios_base::openmode l_mode = std::ios::out;
	switch (saveMode)
	{
	case IOMode::Text:
		l_mode = std::ios::out;
		break;
	case IOMode::Binary:
		l_mode = std::ios::out | std::ios::ate | std::ios::binary;
		break;
	default:
		break;
	}

	std::ofstream l_file;

	l_file.open((m_workingDir + filePath).c_str(), l_mode);

	if (!l_file.is_open())
	{
		InnoLogger::Log(LogLevel::Error, "IOService: can't open file : ", filePath, "!");
		return false;
	}

	auto l_result = serializeVector(l_file, content);

	l_file.close();

	return l_result;
}

bool IOService::isFileExist(const char* filePath)
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

std::string IOService::getFilePath(const char* filePath)
{
	return fs::path(filePath).remove_filename().generic_string();
}

std::string IOService::getFileExtension(const char* filePath)
{
	return fs::path(filePath).extension().generic_string();
}

std::string IOService::getFileName(const char* filePath)
{
	return fs::path(filePath).stem().generic_string();
}

std::string IOService::getWorkingDirectory()
{
	return m_workingDir;
}