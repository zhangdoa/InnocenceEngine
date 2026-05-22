#include "IOService.h"
#include "Array.h"

#include "STL17.h"

#include <filesystem>
namespace fs = std::filesystem;

#include "LogService.h"

#include "../Engine.h"
using namespace Inno;
;

bool IOService::SetupWorkingDirectory()
{
	m_workingDir = fs::current_path().generic_string() + "/";
	m_dataDir = m_workingDir + "../Data/";

	Log(Verbose, "current working directory is ", m_workingDir.c_str());
	Log(Verbose, "data directory is ", m_dataDir.c_str());

	return true;
}

// Absolute inputs pass through verbatim; relative inputs are rooted at workingDir.
static std::string ResolvePath(const std::string& workingDir, const char* filePath)
{
	if (filePath && fs::path(filePath).is_absolute())
		return std::string(filePath);
	return workingDir + (filePath ? filePath : "");
}

Inno::Array<char> IOService::LoadFile(const char* filePath, IOMode openMode)
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

	auto l_resolved = ResolvePath(m_workingDir, filePath);
	l_file.open(l_resolved.c_str(), l_mode);

	if (!l_file.is_open())
	{
		Log(Error, "Can't open file : ", filePath, " (resolved to ", l_resolved.c_str(), ")");
		return Inno::Array<char>();
	}

	auto pbuf = l_file.rdbuf();
	std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
	pbuf->pubseekpos(0, l_file.in);

	Inno::Array<char> buffer(l_size);
	pbuf->sgetn(&buffer[0], l_size);

	l_file.close();

	return buffer;
}

bool IOService::SaveFile(const char* filePath, const Inno::Array<char>& content, IOMode saveMode)
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

	auto l_resolved = ResolvePath(m_workingDir, filePath);
	l_file.open(l_resolved.c_str(), l_mode);

	if (!l_file.is_open())
	{
		Log(Error, "Can't open file : ", filePath, " (resolved to ", l_resolved.c_str(), ")");
		return false;
	}

	auto l_result = SerializeVector(l_file, content);

	l_file.close();

	return l_result;
}

bool IOService::IsFileExist(const char* filePath)
{
	return fs::exists(fs::path(ResolvePath(m_workingDir, filePath)));
}

std::string IOService::GetFilePath(const char* filePath)
{
	return fs::path(filePath).remove_filename().generic_string();
}

std::string IOService::GetFileExtension(const char* filePath)
{
	return fs::path(filePath).extension().generic_string();
}

std::string IOService::GetFileName(const char* filePath)
{
	return fs::path(filePath).stem().generic_string();
}

std::string IOService::GetWorkingDirectory()
{
	return m_workingDir;
}

std::string IOService::GetDataDirectory()
{
	return m_dataDir;
}

std::string IOService::GetEngineDirectory()
{
	return m_dataDir + "Engine/";
}

std::string IOService::GetProjectName()
{
	return INNO_PROJECT_NAME;
}

std::string IOService::GetProjectDirectory()
{
	return m_dataDir + INNO_PROJECT_NAME + std::string("/");
}

std::string IOService::GetGeneratedDirectory()
{
	return m_dataDir + "Generated/";
}

std::string IOService::GetComponentDirectory()
{
	return m_dataDir + "Generated/Components/";
}

std::string IOService::ValidateFileName(const char* filePath)
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

bool IOService::AddCPPClassFiles(const CPPClassDesc& desc)
{
	auto l_headerFileName = desc.filePath + desc.className + ".h";
	std::ofstream l_headerFile(IOService::GetWorkingDirectory() + l_headerFileName, std::ios::out | std::ios::trunc);

	if (!l_headerFile.is_open())
	{
		Log(Error, "std::ofstream: can't open file ", l_headerFileName.c_str(), "!");
		return false;
	}

	l_headerFile << "#pragma once" << std::endl;
	l_headerFile << "#include \"Common/Enum.h\"" << std::endl;
	l_headerFile << "#include \"Common/ClassTemplate.h\"" << std::endl;
	l_headerFile << std::endl;

	if (desc.isInterface)
	{
		l_headerFile << "class ";
	}
	else
	{
		l_headerFile << "class ";
	}

	l_headerFile << desc.className;

	if (!desc.parentClass.empty())
	{
		l_headerFile << " : public " << desc.parentClass;
	}

	l_headerFile << std::endl;

	l_headerFile << "{" << std::endl;
	l_headerFile << "public:" << std::endl;

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

	Log(Success, "", l_headerFileName.c_str(), " has been generated.");
	return true;
}