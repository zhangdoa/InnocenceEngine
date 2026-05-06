#include "AssetService.h"
#include "../Common/LogService.h"
#include "../Common/IOService.h"
#include "../Common/TaskScheduler.h"
#include "../ThirdParty/AssimpWrapper/AssimpWrapper.h"

#include <filesystem>

#include "../Engine.h"
using namespace Inno;

std::string AssetService::GetAssetFilePath(const char* componentName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();
	auto l_name = std::string(componentName) + ".json";

	// Search order: project components, then generated (imported) components
	auto l_projectPath = std::string(INNO_PROJECT_NAME) + "/Components/" + l_name;
	if (std::filesystem::exists(l_dataDir + l_projectPath))
		return l_projectPath;

	auto l_generatedPath = "Generated/Components/" + l_name;
	if (std::filesystem::exists(l_dataDir + l_generatedPath))
		return l_generatedPath;

	// Default to generated (where imports write to)
	return l_generatedPath;
}

std::string AssetService::GetBinaryFilePath(const char* binaryFileName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();

	// Search order: project components, then generated (imported) components
	auto l_projectPath = l_dataDir + INNO_PROJECT_NAME + std::string("/Components/") + binaryFileName;
	if (std::filesystem::exists(l_projectPath))
		return l_projectPath;

	auto l_generatedPath = l_dataDir + "Generated/Components/" + binaryFileName;
	if (std::filesystem::exists(l_generatedPath))
		return l_generatedPath;

	// Default to generated (where imports write to)
	return l_generatedPath;
}

std::string AssetService::GetComponentDirectory()
{
	return g_Engine->Get<IOService>()->getComponentDirectory();
}

bool AssetService::Import(const char* fileName)
{
	auto* l_io = g_Engine->Get<IOService>();
	auto l_extension = l_io->getFileExtension(fileName);
	std::string l_fileName = fileName;

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".ply" || l_extension == ".PLY" || l_extension == ".md5mesh")
	{
		// Each import task runs concurrently — asset registry access is protected per-type by
		// shared_mutex inside Allocate*/Get*/Find* functions.
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Import Model Task", ITask::Type::Once), [=]()
			{
				AssimpWrapper::Import(l_fileName.c_str());
			});
		tempTask->Activate();
		return true;
	}
	else if (l_extension == ".png" || l_extension == ".PNG" || l_extension == ".jpg" || l_extension == ".JPG" || l_extension == ".jpeg" || l_extension == ".JPEG" || l_extension == ".tga" || l_extension == ".TGA")
	{
		// Auto-detect material slot + sRGB from filename suffix using the
		// AmbientCG / common-PBR convention. Unknown suffixes fall through
		// as albedo (slot 1, sRGB) — the safest default for an unlabelled
		// colour image.
		std::string l_baseName = l_io->getFileName(fileName);
		auto l_dot = l_baseName.find_last_of('.');
		if (l_dot != std::string::npos)
			l_baseName.erase(l_dot);
		std::string l_lowered;
		l_lowered.reserve(l_baseName.size());
		for (char c : l_baseName)
			l_lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

		uint32_t l_slot   = 1;
		bool     l_isSRGB = true;
		if (l_lowered.find("normal") != std::string::npos)         { l_slot = 0; l_isSRGB = false; }
		else if (l_lowered.find("metalness") != std::string::npos) { l_slot = 2; l_isSRGB = false; }
		else if (l_lowered.find("metallic")  != std::string::npos) { l_slot = 2; l_isSRGB = false; }
		else if (l_lowered.find("roughness") != std::string::npos) { l_slot = 3; l_isSRGB = false; }
		else if (l_lowered.find("ambientocclusion") != std::string::npos
		      || l_lowered.find("_ao") != std::string::npos)        { l_slot = 4; l_isSRGB = false; }
		else if (l_lowered.find("color")     != std::string::npos
		      || l_lowered.find("albedo")    != std::string::npos
		      || l_lowered.find("basecolor") != std::string::npos)  { l_slot = 1; l_isSRGB = true;  }

		std::string l_instanceName = l_baseName + ".TextureComponent";
		auto tempTask = g_Engine->Get<TaskScheduler>()->Submit(ITask::Desc("Import Texture Task", ITask::Type::Once),
			[fileNameCopy = l_fileName, instanceName = std::move(l_instanceName), l_slot, l_isSRGB]()
			{
				ImportTexture(fileNameCopy.c_str(), TextureSampler::Sampler2D, TextureUsage::Sample, l_isSRGB, l_slot, instanceName.c_str());
			});
		tempTask->Activate();
		return true;
	}
	else
	{
		Log(Warning, fileName, " is not supported!");

		return false;
	}
}

bool AssetService::ImportSync(const char* fileName)
{
	auto l_extension = g_Engine->Get<IOService>()->getFileExtension(fileName);

	if (l_extension == ".obj" || l_extension == ".OBJ" || l_extension == ".fbx" || l_extension == ".FBX" || l_extension == ".gltf" || l_extension == ".GLTF" || l_extension == ".ply" || l_extension == ".PLY" || l_extension == ".md5mesh")
	{
		// Offline/synchronous import: run directly on the calling thread with no scheduler involvement.
		// This avoids task queue races during engine teardown and is the correct semantic for tools/tests.
		return AssimpWrapper::Import(fileName);
	}
	else
	{
		Log(Warning, fileName, " is not supported!");
		return false;
	}
}
