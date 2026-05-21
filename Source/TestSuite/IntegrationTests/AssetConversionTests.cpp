// CPU-only end-to-end exercise of AssetService::ImportSync(). No GPU or window services needed.
#include "../Common/TestRunner.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/Common/IOService.h"
#include "../../Engine/Engine.h"

#include <filesystem>
#include <fstream>

using namespace Inno;
namespace fs = std::filesystem;

static std::string GetTempAssetDir()
{
	return g_Engine->Get<IOService>()->getWorkingDirectory() + "../Build/asset_test_tmp/";
}

static void WriteMiniOBJ(const std::string& path)
{
	fs::create_directories(fs::path(path).parent_path());
	std::ofstream f(path);
	f << "# InnocenceEngine asset conversion test fixture\n";
	f << "v  0.0  0.0  0.0\n";
	f << "v  1.0  0.0  0.0\n";
	f << "v  0.5  1.0  0.0\n";
	f << "f 1 2 3\n";
}

static void CleanupTempAssets(const std::string& stemName)
{
	fs::remove_all(GetTempAssetDir());

	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();
	auto l_componentsDir = fs::path(l_dataDir + "Generated/Components/");
	auto l_scenesDir = fs::path(l_dataDir + "Generated/Scenes/");

	fs::remove(l_scenesDir / (stemName + ".InnoScene"));

	// Components live in subdirectories named after the mesh component instance
	// (e.g. Data/Generated/Components/test_triangle.0.MeshComponent/).
	for (auto& entry : fs::directory_iterator(l_componentsDir))
	{
		if (entry.path().filename().string().find(stemName) != std::string::npos)
			fs::remove_all(entry.path());
	}
}

static bool SceneFileExists(const std::string& stemName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();
	auto l_path = fs::path(l_dataDir + "Generated/Scenes/" + stemName + ".InnoScene");
	return fs::exists(l_path);
}

static bool AnyComponentFileExists(const std::string& stemName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();
	auto l_componentsDir = fs::path(l_dataDir + "Generated/Components/");
	if (!fs::exists(l_componentsDir))
		return false;
	for (auto& entry : fs::recursive_directory_iterator(l_componentsDir))
	{
		if (entry.path().filename().string().find(stemName) != std::string::npos)
			return true;
	}
	return false;
}

// Texture files: "<stem>.<textureName>.json" under Components/.
static bool AnyTextureComponentExists(const std::string& stemName)
{
	auto l_dataDir = g_Engine->Get<IOService>()->getDataDirectory();
	auto l_componentsDir = fs::path(l_dataDir + "Generated/Components/");
	if (!fs::exists(l_componentsDir))
		return false;
	for (auto& entry : fs::recursive_directory_iterator(l_componentsDir))
	{
		auto l_name = entry.path().filename().string();
		if (l_name.find(stemName) != std::string::npos &&
		    l_name.find(".innobin") == std::string::npos &&
		    l_name.find("MeshComponent") == std::string::npos &&
		    l_name.find("MaterialComponent") == std::string::npos &&
		    l_name.find(".json") != std::string::npos)
			return true;
	}
	return false;
}

static void TestImportUnsupportedExtension()
{
	TestRunner::StartTest("Import: unsupported extension returns false");

	bool l_result = AssetService::ImportSync("some_model.xyz");

	TestRunner::EndTest(!l_result);
}

static void TestImportNonexistentFile()
{
	TestRunner::StartTest("Import: nonexistent file returns false");

	bool l_result = AssetService::ImportSync("../OriginalAssets/Models/__no_such_file__.obj");

	TestRunner::EndTest(!l_result);
}

static void TestImportMinimalOBJ()
{
	TestRunner::StartTest("Import: minimal OBJ produces scene and component outputs");

	const std::string l_stemName = "test_triangle";
	const std::string l_tempPath = GetTempAssetDir() + l_stemName + ".obj";
	const std::string l_relPath = "../Build/asset_test_tmp/" + l_stemName + ".obj";

	CleanupTempAssets(l_stemName);
	WriteMiniOBJ(l_tempPath);

	bool l_importOk = AssetService::ImportSync(l_relPath.c_str());
	bool l_sceneExists = SceneFileExists(l_stemName);
	bool l_componentExists = AnyComponentFileExists(l_stemName);

	bool l_passed = l_importOk && l_sceneExists && l_componentExists;

	if (!l_importOk)
		Log(Error, "AssetConversionTests: ImportSync returned false for minimal OBJ");
	if (!l_sceneExists)
		Log(Error, "AssetConversionTests: Generated scene file not found for: ", l_stemName.c_str());
	if (!l_componentExists)
		Log(Error, "AssetConversionTests: No generated component files found for: ", l_stemName.c_str());

	CleanupTempAssets(l_stemName);

	TestRunner::EndTest(l_passed);
}

static void TestImportPLYConditional()
{
	const char* l_relPath = "../OriginalAssets/Models/bunny/bunny.ply";

	if (!g_Engine->Get<IOService>()->isFileExist(l_relPath))
	{
		TestRunner::StartTest("Import: PLY (bunny) — SKIPPED (OriginalAssets not present)");
		TestRunner::EndTest(true);
		return;
	}

	TestRunner::StartTest("Import: PLY (bunny) produces scene and component outputs");

	bool l_importOk = AssetService::ImportSync(l_relPath);
	bool l_sceneExists = SceneFileExists("bunny");
	bool l_componentExists = AnyComponentFileExists("bunny");

	bool l_passed = l_importOk && l_sceneExists && l_componentExists;

	if (!l_importOk)
		Log(Error, "AssetConversionTests: ImportSync returned false for bunny.ply");
	if (!l_sceneExists)
		Log(Error, "AssetConversionTests: Generated scene file not found for: bunny");
	if (!l_componentExists)
		Log(Error, "AssetConversionTests: No generated component files found for: bunny");

	TestRunner::EndTest(l_passed);
}

// Full pipeline: mesh → material → texture load → BC compression → disk write.
static void TestImportTexturedFBXConditional()
{
	const char* l_relPath = "../OriginalAssets/Models/orb/ShaderBall.fbx";
	const std::string l_stemName = "ShaderBall";

	if (!g_Engine->Get<IOService>()->isFileExist(l_relPath))
	{
		TestRunner::StartTest("Import: FBX with textures (ShaderBall) — SKIPPED (OriginalAssets not present)");
		TestRunner::EndTest(true);
		return;
	}

	TestRunner::StartTest("Import: FBX with textures (ShaderBall) produces scene, mesh, material, and texture outputs");

	bool l_importOk = AssetService::ImportSync(l_relPath);
	bool l_sceneExists = SceneFileExists(l_stemName);
	bool l_componentExists = AnyComponentFileExists(l_stemName);
	bool l_textureExists = AnyTextureComponentExists(l_stemName);

	bool l_passed = l_importOk && l_sceneExists && l_componentExists && l_textureExists;

	if (!l_importOk)
		Log(Error, "AssetConversionTests: ImportSync returned false for ShaderBall.fbx");
	if (!l_sceneExists)
		Log(Error, "AssetConversionTests: Generated scene file not found for: ", l_stemName.c_str());
	if (!l_componentExists)
		Log(Error, "AssetConversionTests: No generated component files found for: ", l_stemName.c_str());
	if (!l_textureExists)
		Log(Error, "AssetConversionTests: No generated texture component files found for: ", l_stemName.c_str());

	TestRunner::EndTest(l_passed);
}

void RunAssetConversionTests()
{
	TestRunner::StartTestSuite("Asset Conversion (Integration)");

	TestImportUnsupportedExtension();
	TestImportNonexistentFile();
	TestImportMinimalOBJ();
	TestImportPLYConditional();
	TestImportTexturedFBXConditional();

	TestRunner::EndTestSuite();
}
