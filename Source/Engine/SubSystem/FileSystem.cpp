#include "FileSystem.h"
#include "../Core/Logger.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

#include "../Core/IOService.h"

namespace FileSystemNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

using namespace FileSystemNS;

bool FileSystem::Setup(ISystemConfig* systemConfig)
{
	IOService::setupWorkingDirectory();

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool FileSystem::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Logger::Log(LogLevel::Success, "FileSystem has been initialized.");
		return true;
	}
	else
	{
		Logger::Log(LogLevel::Error, "FileSystem: Object is not created!");
		return false;
	}
}

bool FileSystem::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	return true;
}

bool FileSystem::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus FileSystem::GetStatus()
{
	return m_ObjectStatus;
}

std::string FileSystem::getWorkingDirectory()
{
	return IOService::getWorkingDirectory();
}

std::vector<char> FileSystem::loadFile(const char* filePath, IOMode openMode)
{
	return IOService::loadFile(filePath, openMode);
}

bool FileSystem::saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode)
{
	return IOService::saveFile(filePath, content, saveMode);
}

bool FileSystem::addCPPClassFiles(const CPPClassDesc& desc)
{
	// Build header file
	auto l_headerFileName = desc.filePath + desc.className + ".h";
	std::ofstream l_headerFile(IOService::getWorkingDirectory() + l_headerFileName, std::ios::out | std::ios::trunc);

	if (!l_headerFile.is_open())
	{
		Logger::Log(LogLevel::Error, "FileSystem: std::ofstream: can't open file ", l_headerFileName.c_str(), "!");
		return false;
	}

	// Common headers include
	l_headerFile << "#pragma once" << std::endl;
	l_headerFile << "#include \"Common/Type.h\"" << std::endl;
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
			l_headerFile << "  INNO_CLASS_CONCRETE_DEFALUT(" << desc.className << ");" << std::endl;
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

	Logger::Log(LogLevel::Success, "FileSystem: ", l_headerFileName.c_str(), " has been generated.");
	return true;
}