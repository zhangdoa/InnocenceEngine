#include "FileSystem.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

#include "FileSystemHelper.h"
#include "AssimpWrapper.h"
#include "AssetLoader.h"
#include "JSONParser.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	bool convertModel(const std::string & fileName, const std::string & exportPath);

	bool saveScene(const std::string& fileName);
	bool prepareForLoadingScene(const std::string& fileName);
	bool loadScene(const std::string& fileName);

	bool loadAssets();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::vector<InnoFuture<void>> m_asyncTask;
	std::vector<std::function<void()>*> m_sceneLoadingStartCallbacks;
	std::vector<std::function<void()>*> m_sceneLoadingFinishCallbacks;
	std::function<void()> f_convertModelTest;
	std::function<void()> f_loadModelTest;

	std::atomic<bool> m_isLoadingScene = false;
	std::atomic<bool> m_prepareForLoadingScene = false;

	std::string m_nextLoadingScene;
	std::string m_currentScene;
}

bool InnoFileSystemNS::convertModel(const std::string & fileName, const std::string & exportPath)
{
	auto l_extension = getFileExtension(fileName);

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX")
	{
		auto tempTask = g_pCoreSystem->getTaskSystem()->submit([=]()
		{
			AssimpWrapper::convertModel(fileName, exportPath);
		});
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: " + fileName + " is not supported!");

		return false;
	}

	return true;
}

bool InnoFileSystemNS::saveScene(const std::string& fileName)
{
	return JSONParser::saveScene(fileName);
}

bool InnoFileSystemNS::prepareForLoadingScene(const std::string& fileName)
{
	if (!InnoFileSystemNS::m_isLoadingScene)
	{
		if (m_currentScene == fileName)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "FileSystem: scene " + fileName + " has already loaded now.");
			return true;
		}
		m_nextLoadingScene = fileName;
		m_prepareForLoadingScene = true;
	}

	return true;
}

bool InnoFileSystemNS::loadScene(const std::string& fileName)
{
	for (auto i : m_sceneLoadingStartCallbacks)
	{
		(*i)();
	}

	JSONParser::loadScene(fileName);

	for (auto i : m_sceneLoadingFinishCallbacks)
	{
		(*i)();
	}

	InnoFileSystemNS::m_isLoadingScene = false;

	m_currentScene = fileName;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: scene " + fileName + " has been loaded.");

	loadAssets();

	return true;
}

bool InnoFileSystemNS::loadAssets()
{
	g_pCoreSystem->getAssetSystem()->loadAssetsForComponents();

	return true;
}

bool InnoFileSystem::setup()
{
	InnoFileSystemNS::f_convertModelTest = [&]() { convertModel("res/models/Robot/Robot.FBX", "res/test/"); };
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_Y, ButtonStatus::PRESSED }, &InnoFileSystemNS::f_convertModelTest);

	InnoFileSystemNS::f_loadModelTest = [&]() { loadModel("res/test/Robot.InnoModel"); };
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_U, ButtonStatus::PRESSED }, &InnoFileSystemNS::f_loadModelTest);

	InnoFileSystemNS::setup();
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "FileSystem: current working directory is " + getWorkingDirectory());
	InnoFileSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

bool InnoFileSystem::initialize()
{
	return true;
}

bool InnoFileSystem::update()
{
	if (InnoFileSystemNS::m_prepareForLoadingScene)
	{
		InnoFileSystemNS::m_prepareForLoadingScene = false;
		InnoFileSystemNS::m_isLoadingScene = true;
		g_pCoreSystem->getTaskSystem()->waitAllTasksToFinish();
		InnoFileSystemNS::loadScene(InnoFileSystemNS::m_nextLoadingScene);
	}

	return true;
}

bool InnoFileSystem::terminate()
{
	InnoFileSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;

	return true;
}

ObjectStatus InnoFileSystem::getStatus()
{
	return InnoFileSystemNS::m_objectStatus;
}

std::string InnoFileSystem::getWorkingDirectory()
{
	return InnoFileSystemNS::getWorkingDirectory();
}

std::string InnoFileSystem::loadTextFile(const std::string & fileName)
{
	return InnoFileSystemNS::loadTextFile(fileName);
}

std::vector<char> InnoFileSystem::loadBinaryFile(const std::string & fileName)
{
	return InnoFileSystemNS::loadBinaryFile(fileName);
}

bool InnoFileSystem::loadScene(const std::string & fileName)
{
	return InnoFileSystemNS::prepareForLoadingScene(fileName);
}

bool InnoFileSystem::saveScene(const std::string & fileName)
{
	return InnoFileSystemNS::saveScene(fileName);
}

bool InnoFileSystem::isLoadingScene()
{
	return InnoFileSystemNS::m_isLoadingScene;
}

bool InnoFileSystem::addSceneLoadingStartCallback(std::function<void()>* functor)
{
	InnoFileSystemNS::m_sceneLoadingStartCallbacks.emplace_back(functor);
	return true;
}

bool InnoFileSystem::addSceneLoadingFinishCallback(std::function<void()>* functor)
{
	InnoFileSystemNS::m_sceneLoadingFinishCallbacks.emplace_back(functor);
	return true;
}

bool InnoFileSystem::convertModel(const std::string & fileName, const std::string & exportPath)
{
	return InnoFileSystemNS::convertModel(fileName, exportPath);
}

ModelMap InnoFileSystem::loadModel(const std::string & fileName)
{
	return InnoFileSystemNS::AssetLoader::loadModel(fileName);
}

TextureDataComponent* InnoFileSystem::loadTexture(const std::string & fileName)
{
	return InnoFileSystemNS::AssetLoader::loadTexture(fileName);
}

bool InnoFileSystem::addCPPClassFiles(const CPPClassDesc & desc)
{
	// Build header file
	auto l_headerFileName = desc.filePath + desc.className + ".h";
	std::ofstream l_headerFile(InnoFileSystemNS::getWorkingDirectory() + l_headerFileName, std::ios::out | std::ios::trunc);

	if (!l_headerFile.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: std::ofstream: can't open file " + l_headerFileName + "!");
		return false;
	}

	// Common headers include
	l_headerFile << "#pragma once" << std::endl;
	l_headerFile << "#include \"common/InnoType.h\"" << std::endl;
	l_headerFile << "#include \"common/InnoClassTemplate.h\"" << std::endl;
	l_headerFile << std::endl;

	// Abstraction type
	if (desc.isInterface)
	{
		l_headerFile << "INNO_INTERFACE ";
	}
	else
	{
		l_headerFile << "INNO_CONCRETE ";
	}

	l_headerFile << desc.className;

	// Inheriance type
	if (!desc.parentClass.empty())
	{
		l_headerFile << " : INNO_IMPLEMENT " << desc.parentClass;
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
	l_headerFile << "  bool setup();" << std::endl;
	l_headerFile << "  bool initialize();" << std::endl;
	l_headerFile << "  bool update();" << std::endl;
	l_headerFile << "  bool terminate();" << std::endl;
	l_headerFile << "  ObjectStatus getStatus();" << std::endl;

	l_headerFile << std::endl;
	l_headerFile << "private:" << std::endl;
	l_headerFile << "  ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;" << std::endl;
	l_headerFile << "};" << std::endl;

	l_headerFile.close();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem: " + l_headerFileName + " has been generated.");
	return true;
}