#include "FileSystem.h"

#include <fstream>
#include <sstream>

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"

#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLFrameBufferComponent.h"
#include "../component/GLShaderProgramComponent.h"
#include "../component/GLRenderPassComponent.h"

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"
#include "../component/DXShaderProgramComponent.h"
#include "../component/DXRenderPassComponent.h"
#endif

#include "../component/PhysicsDataComponent.h"

#include"../component/AssetSystemComponent.h"

#include "../../engine/system/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	bool serialize(std::ostream& os, void* ptr, size_t size)
	{
		os.write((char*)ptr, size);
		return true;
	}

	template<typename T>
	bool serialize(std::ostream& os, const std::vector<T>& vector)
	{
		serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		return true;
	}

	bool saveToDisk(const MeshDataComponent* MDC)
	{
		auto fileName = std::to_string(MDC->m_parentEntity);
		std::ofstream l_file(fileName + ".InnoMesh", std::ios::binary);
		serialize(l_file, MDC->m_vertices);
		l_file.close();

		return true;
	}

	bool saveToDisk(const TextureDataComponent* TDC)
	{
		auto fileName = std::to_string(TDC->m_parentEntity);
		std::ofstream l_file(fileName + ".InnoTexture", std::ios::binary);
		for (auto i : TDC->m_textureData)
		{
			serialize(l_file, i, TDC->m_textureDataDesc.textureWidth * TDC->m_textureDataDesc.textureHeight);
		}
		l_file.close();

		return true;
	}

	template<typename T>
	auto deserialize(const std::string& fileName) -> std::vector<T>
	{
		std::ifstream l_file(fileName, std::ios::binary);

		// get pointer to associated buffer object
		std::filebuf* pbuf = l_file.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
		pbuf->pubseekpos(0, l_file.in);

		std::vector<T> l_result(l_size / sizeof(T));
		pbuf->sgetn((char*)&l_result[0], l_size);

		auto x = l_result;

		l_file.close();
		return l_result;
	}
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::setup()
{
	InnoFileSystemNS::m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::update()
{
	static bool cond = true;
	if (cond)
	{
		auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
		InnoFileSystemNS::saveToDisk(l_MDC);
		auto l_TDC = g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO);
		InnoFileSystemNS::saveToDisk(l_TDC);
		saveComponentToDisk(l_MDC, "testC");
		auto t = loadComponentFromDisk<MeshDataComponent>("testC");
		cond = false;
	}
	return true;
}

INNO_SYSTEM_EXPORT bool InnoFileSystem::terminate()
{
	InnoFileSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;

	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoFileSystem::getStatus()
{
	return InnoFileSystemNS::m_objectStatus;
}

std::string InnoFileSystem::loadTextFile(const std::string & fileName)
{
	std::ifstream file;
	file.open((AssetSystemComponent::get().m_shaderRelativePath + fileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
}

void InnoFileSystem::saveComponentToDiskImpl(componentType type, size_t classSize, void* ptr, const std::string& fileName)
{
	if (!ptr) return;
	char* l_ptr_raw = reinterpret_cast<char*>(ptr);
	unsigned int l_engineVer = 7;
	auto l_time = g_pCoreSystem->getTimeSystem()->getCurrentTime();

	std::ofstream l_file;
	l_file.open(fileName + ".InnoAsset", std::ios::out | std::ios::trunc | std::ios::binary);
	l_file.write((char*)&type, sizeof(type));
	l_file.write((char*)&l_engineVer, sizeof(l_engineVer));
	l_file.write((char*)&l_time, sizeof(l_time));
	l_file.write(l_ptr_raw, classSize);

	l_file.close();
}

void* InnoFileSystem::loadComponentFromDiskImpl(const std::string& fileName)
{
	std::ifstream l_file;
	l_file.open(fileName + ".InnoAsset", std::ios::binary);

	if (!l_file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: Can't open file " + fileName + " for deserialization!");
		return nullptr;
	}

	// get pointer to associated buffer object
	std::filebuf* pbuf = l_file.rdbuf();
	// get file size using buffer's members
	std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
	pbuf->pubseekpos(0, l_file.in);

	// allocate memory to contain file data
	// @TODO: new???
	char* buffer = new char[l_size];

	// get file data
	pbuf->sgetn(buffer, l_size);

	char* l_ptr;
	auto l_classType = *(componentType*)buffer;
	size_t classSize;

	switch (l_classType)
	{
	case componentType::TransformComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<TransformComponent>());
		classSize = sizeof(TransformComponent);
		break;
	case componentType::VisibleComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<VisibleComponent>());
		classSize = sizeof(VisibleComponent);
		break;
	case componentType::DirectionalLightComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<DirectionalLightComponent>());
		classSize = sizeof(DirectionalLightComponent);
		break;
	case componentType::PointLightComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<PointLightComponent>());
		classSize = sizeof(PointLightComponent);
		break;
	case componentType::SphereLightComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<SphereLightComponent>());
		classSize = sizeof(SphereLightComponent);
		break;
	case componentType::CameraComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<CameraComponent>());
		classSize = sizeof(CameraComponent);
		break;
	case componentType::InputComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<InputComponent>());
		classSize = sizeof(InputComponent);
		break;
	case componentType::EnvironmentCaptureComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<EnvironmentCaptureComponent>());
		classSize = sizeof(EnvironmentCaptureComponent);
		break;
	case componentType::PhysicsDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<PhysicsDataComponent>());
		classSize = sizeof(PhysicsDataComponent);
		break;
	case componentType::MeshDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<MeshDataComponent>());
		classSize = sizeof(MeshDataComponent);
		break;
	case componentType::MaterialDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<MaterialDataComponent>());
		classSize = sizeof(MaterialDataComponent);
		break;
	case componentType::TextureDataComponent:
		l_ptr = reinterpret_cast<char*>(g_pCoreSystem->getMemorySystem()->spawn<TextureDataComponent>());
		classSize = sizeof(TextureDataComponent);
		break;
	default:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "FileSystem: Unsupported deserialization data type " + std::to_string((int)l_classType) + " !");
		return nullptr;
		break;
	}

	std::memcpy(l_ptr, buffer + 16, classSize);

	l_file.close();

	delete buffer;

	return l_ptr;
}