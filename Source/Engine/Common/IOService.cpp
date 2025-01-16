#include "IOService.h"

#include "STL17.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "Logger.h"

#include "../Engine.h"
using namespace Inno;
;

bool IOService::setupWorkingDirectory()
{
	m_workingDir = fs::current_path().generic_string();
	m_workingDir = m_workingDir + "//";

	g_Engine->Get<Logger>()->Log(LogLevel::Verbose, "IOService: current working directory is ", m_workingDir.c_str());

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
		g_Engine->Get<Logger>()->Log(LogLevel::Error, "IOService: Can't open file : ", filePath, "!");
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
		g_Engine->Get<Logger>()->Log(LogLevel::Error, "IOService: Can't open file : ", filePath, "!");
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

std::string IOService::validateFileName(const char* filePath)
{
	std::string l_result(filePath);
	std::replace(l_result.begin(), l_result.end(), '|', '-');
	std::replace(l_result.begin(), l_result.end(), '\\', '-');
	std::replace(l_result.begin(), l_result.end(), '/', '-');
	std::replace(l_result.begin(), l_result.end(), '\"', '-');
	std::replace(l_result.begin(), l_result.end(), '<', '-');
	std::replace(l_result.begin(), l_result.end(), '>', '-');
	std::replace(l_result.begin(), l_result.end(), ':', '-');
	std::replace(l_result.begin(), l_result.end(), '*', '-');
	std::replace(l_result.begin(), l_result.end(), '?', '-');
	return l_result;
}

bool IOService::addCPPClassFiles(const CPPClassDesc& desc)
{
	// Build header file
	auto l_headerFileName = desc.filePath + desc.className + ".h";
	std::ofstream l_headerFile(IOService::getWorkingDirectory() + l_headerFileName, std::ios::out | std::ios::trunc);

	if (!l_headerFile.is_open())
	{
		g_Engine->Get<Logger>()->Log(LogLevel::Error, "FileSystem: std::ofstream: can't open file ", l_headerFileName.c_str(), "!");
		return false;
	}

	// Common headers include
	l_headerFile << "#pragma once" << std::endl;
	l_headerFile << "#include \"Common/Enum.h\"" << std::endl;
	l_headerFile << "#include \"Common/ClassTemplate.h\"" << std::endl;
	l_headerFile << std::endl;

	// Abstraction type
	if (desc.isInterface)
	{
		l_headerFile << "class ";
	}
	else
	{
		l_headerFile << "class ";
	}

	l_headerFile << desc.className;

	// Inheriance type
	if (!desc.parentClass.empty())
	{
		l_headerFile << " : public " << desc.parentClass;
	}

	l_headerFile << std::endl;

	// Class decl body
	l_headerFile << "{" << std::endl;
	l_headerFile << "public:" << std::endl;

	// Ctor type
	if (desc.isInterface)
	{
		if (desc.isNonMoveable && desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_COPYABLE_AND_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonMoveable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_NON_COPYABLE(" << desc.className << ");" << std::endl;
		}
		else
		{
			l_headerFile << "  INNO_CLASS_INTERFACE_DEFALUT(" << desc.className << ");" << std::endl;
		}
	}
	else
	{
		if (desc.isNonMoveable && desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_COPYABLE_AND_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonMoveable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_MOVABLE(" << desc.className << ");" << std::endl;
		}
		else if (desc.isNonCopyable)
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_NON_COPYABLE(" << desc.className << ");" << std::endl;
		}
		else
		{
			l_headerFile << "  INNO_CLASS_CONCRETE_DEFAULT(" << desc.className << ");" << std::endl;
		}
	}

	l_headerFile << std::endl;
	l_headerFile << "  bool Setup();" << std::endl;
	l_headerFile << "  bool Initialize();" << std::endl;
	l_headerFile << "  bool Update();" << std::endl;
	l_headerFile << "  bool Terminate();" << std::endl;
	l_headerFile << "  ObjectStatus GetStatus();" << std::endl;

	l_headerFile << std::endl;
	l_headerFile << "private:" << std::endl;
	l_headerFile << "  ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;" << std::endl;
	l_headerFile << "};" << std::endl;

	l_headerFile.close();

	g_Engine->Get<Logger>()->Log(LogLevel::Success, "FileSystem: ", l_headerFileName.c_str(), " has been generated.");
	return true;
}