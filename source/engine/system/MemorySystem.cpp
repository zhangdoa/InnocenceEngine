#include "MemorySystem.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

//Double-linked-list
class freeChunk
{
public:
	void* m_target = nullptr;
	freeChunk* m_next = nullptr;
	freeChunk* m_prev = nullptr;
};

template <class T, unsigned long long TCapability>
class objectPool
{
public:
	objectPool() : objectPool(TCapability)
	{
	};

	objectPool(unsigned long long capability)
	{
		m_capability = capability;
		m_poolSize = capability * sizeof(T);
		m_poolPtr = ::new unsigned char[m_poolSize];
	};

	~objectPool() {
		::delete[] m_poolPtr;
	};

	unsigned long long m_capability = 0;
	unsigned long long m_poolSize = 0;
	unsigned char* m_poolPtr = nullptr;
};

class MemoryWatchdog
{
public:
	static MemoryWatchdog& get()
	{
		static MemoryWatchdog instance;

		return instance;
	}

	bool recordRawMemoryUsage(void* ptr, size_t size)
	{
		auto l_result = m_memo.find(ptr);
		if (l_result != m_memo.end())
		{
			std::stringstream ss;
			ss << ptr;
			std::string name = ss.str();
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: MemoryWatchdog: allocate collision happened at " + name + " !");
			return false;
		}
		else
		{
			m_memo.emplace(ptr, size);
			return true;
		}
	}

private:
	std::unordered_map<void*, size_t> m_memo;
};

INNO_PRIVATE_SCOPE InnoMemorySystemNS
{
#define allocateInitialFreeChucksDefi( className ) \
bool allocateInitialFreeChucksFor##className() \
{ \
	auto l_chuckUC = m_##className##FreeChunkPool->m_poolPtr; \
	auto l_componentUC = m_##className##Pool->m_poolPtr; \
\
	freeChunk* l_prevFreeChunk = nullptr; \
\
	/* walk through others */ \
	for (unsigned long long i = 0; i < m_##className##FreeChunkPool->m_capability; i++) \
	{ \
		auto l_newFreeChunk = new(l_chuckUC) freeChunk(); \
 \
		l_newFreeChunk->m_target = l_componentUC; \
		l_newFreeChunk->m_prev = l_prevFreeChunk; \
		if(l_prevFreeChunk) \
		{ \
			l_newFreeChunk->m_prev->m_next = l_newFreeChunk; \
		} \
\
		l_prevFreeChunk = l_newFreeChunk; \
		l_chuckUC += sizeof(freeChunk); \
		l_componentUC += sizeof(className); \
	} \
\
	m_##className##CurrentFreeChunk = reinterpret_cast<freeChunk*>(m_##className##FreeChunkPool->m_poolPtr); \
\
	return true; \
}

#define objectPoolUniPtr( className, size ) \
std::unique_ptr<objectPool<className, size>> m_##className##Pool = std::make_unique<objectPool<className, size>>(); \
std::unique_ptr<objectPool<freeChunk, size>> m_##className##FreeChunkPool = std::make_unique<objectPool<freeChunk, size>>(); \
freeChunk* m_##className##CurrentFreeChunk; \
allocateInitialFreeChucksDefi(className)

	// Memory pool for components
	objectPoolUniPtr(TransformComponent, 16384);
	objectPoolUniPtr(VisibleComponent, 16384);
	objectPoolUniPtr(DirectionalLightComponent, 16384);
	objectPoolUniPtr(PointLightComponent, 16384);
	objectPoolUniPtr(SphereLightComponent, 16384);
	objectPoolUniPtr(CameraComponent, 16384);
	objectPoolUniPtr(InputComponent, 16384);
	objectPoolUniPtr(EnvironmentCaptureComponent, 16384);

	objectPoolUniPtr(MeshDataComponent, 16384);
	objectPoolUniPtr(MaterialDataComponent, 16384);
	objectPoolUniPtr(TextureDataComponent, 16384);

	objectPoolUniPtr(GLMeshDataComponent, 16384);
	objectPoolUniPtr(GLTextureDataComponent, 16384);
	objectPoolUniPtr(GLFrameBufferComponent, 16384);
	objectPoolUniPtr(GLShaderProgramComponent, 16384);
	objectPoolUniPtr(GLRenderPassComponent, 16384);

	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	objectPoolUniPtr(DXMeshDataComponent, 16384);
	objectPoolUniPtr(DXTextureDataComponent, 16384);
	objectPoolUniPtr(DXShaderProgramComponent, 16384);
	objectPoolUniPtr(DXRenderPassComponent, 16384);
	#endif

	objectPoolUniPtr(PhysicsDataComponent, 16384);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	bool setup();
}

bool InnoMemorySystemNS::setup()
{
#define constructObjectPool( className, size ) \
	allocateInitialFreeChucksFor##className();

	constructObjectPool(TransformComponent, 16384);
	constructObjectPool(VisibleComponent, 16384);
	constructObjectPool(DirectionalLightComponent, 16384);
	constructObjectPool(PointLightComponent, 16384);
	constructObjectPool(SphereLightComponent, 16384);
	constructObjectPool(CameraComponent, 16384);
	constructObjectPool(InputComponent, 16384);
	constructObjectPool(EnvironmentCaptureComponent, 16384);

	constructObjectPool(MeshDataComponent, 16384);
	constructObjectPool(MaterialDataComponent, 16384);
	constructObjectPool(TextureDataComponent, 16384);

	constructObjectPool(GLMeshDataComponent, 16384);
	constructObjectPool(GLTextureDataComponent, 16384);
	constructObjectPool(GLFrameBufferComponent, 16384);
	constructObjectPool(GLShaderProgramComponent, 16384);
	constructObjectPool(GLRenderPassComponent, 16384);

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	constructObjectPool(DXMeshDataComponent, 16384);
	constructObjectPool(DXTextureDataComponent, 16384);
	constructObjectPool(DXShaderProgramComponent, 16384);
	constructObjectPool(DXRenderPassComponent, 16384);
#endif

	constructObjectPool(PhysicsDataComponent, 16384);

	return true;
}

INNO_SYSTEM_EXPORT bool InnoMemorySystem::setup()
{
	return InnoMemorySystemNS::setup();
}

INNO_SYSTEM_EXPORT bool InnoMemorySystem::initialize()
{
	InnoMemorySystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MemorySystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoMemorySystem::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnoMemorySystem::terminate()
{
	InnoMemorySystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MemorySystem has been terminated.");
	return true;
}

#define allocateComponentImplDefi( className ) \
className* InnoMemorySystem::allocate##className() \
{ \
	auto l_ptr = new(InnoMemorySystemNS::m_##className##CurrentFreeChunk->m_target) className(); \
	if (l_ptr) \
	{ \
		auto l_next = InnoMemorySystemNS::m_##className##CurrentFreeChunk->m_next; \
		if(l_next) \
		{ \
			l_next->m_prev = nullptr; \
			InnoMemorySystemNS::m_##className##CurrentFreeChunk = l_next; \
		} \
		else \
		{ \
			InnoMemorySystemNS::m_##className##CurrentFreeChunk = nullptr; \
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Run out of memory pool for " + std::string(#className) + " !"); \
		} \
		return l_ptr; \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Can't allocate memory for " + std::string(#className) + " !"); \
		return nullptr; \
	} \
}

allocateComponentImplDefi(TransformComponent)
allocateComponentImplDefi(VisibleComponent)
allocateComponentImplDefi(DirectionalLightComponent)
allocateComponentImplDefi(PointLightComponent)
allocateComponentImplDefi(SphereLightComponent)
allocateComponentImplDefi(CameraComponent)
allocateComponentImplDefi(InputComponent)
allocateComponentImplDefi(EnvironmentCaptureComponent)

allocateComponentImplDefi(MeshDataComponent)
allocateComponentImplDefi(MaterialDataComponent)
allocateComponentImplDefi(TextureDataComponent)

allocateComponentImplDefi(GLMeshDataComponent)
allocateComponentImplDefi(GLTextureDataComponent)
allocateComponentImplDefi(GLFrameBufferComponent)
allocateComponentImplDefi(GLShaderProgramComponent)
allocateComponentImplDefi(GLRenderPassComponent)

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
allocateComponentImplDefi(DXMeshDataComponent)
allocateComponentImplDefi(DXTextureDataComponent)
allocateComponentImplDefi(DXShaderProgramComponent)
allocateComponentImplDefi(DXRenderPassComponent)
#endif

allocateComponentImplDefi(PhysicsDataComponent)

#define freeComponentImplDefi( className ) \
bool InnoMemorySystem::free##className(className* p) \
{ \
	/* get pointer distance between this object and the head of the pool*/ \
	auto l_offset = reinterpret_cast<unsigned char*>(p) - InnoMemorySystemNS::m_##className##Pool->m_poolPtr; \
	auto l_index = l_offset / sizeof(className); \
	auto l_freeChuck = new(InnoMemorySystemNS::m_##className##FreeChunkPool->m_poolPtr + l_index * sizeof(freeChunk)) freeChunk(); \
	/* now insert after the current free chunk*/ \
	l_freeChuck->m_target = p; \
	l_freeChuck->m_prev = InnoMemorySystemNS::m_##className##CurrentFreeChunk; \
	l_freeChuck->m_next = InnoMemorySystemNS::m_##className##CurrentFreeChunk->m_next; \
	InnoMemorySystemNS::m_##className##CurrentFreeChunk->m_next = l_freeChuck; \
	/*finally wipe away all the old data*/ \
	std::memset(p, 0, sizeof(className)); \
\
	return true; \
} \

freeComponentImplDefi(TransformComponent)
freeComponentImplDefi(VisibleComponent)
freeComponentImplDefi(DirectionalLightComponent)
freeComponentImplDefi(PointLightComponent)
freeComponentImplDefi(SphereLightComponent)
freeComponentImplDefi(CameraComponent)
freeComponentImplDefi(InputComponent)
freeComponentImplDefi(EnvironmentCaptureComponent)

freeComponentImplDefi(MeshDataComponent)
freeComponentImplDefi(MaterialDataComponent)
freeComponentImplDefi(TextureDataComponent)

freeComponentImplDefi(GLMeshDataComponent)
freeComponentImplDefi(GLTextureDataComponent)
freeComponentImplDefi(GLFrameBufferComponent)
freeComponentImplDefi(GLShaderProgramComponent)
freeComponentImplDefi(GLRenderPassComponent)

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
freeComponentImplDefi(DXMeshDataComponent)
freeComponentImplDefi(DXTextureDataComponent)
freeComponentImplDefi(DXShaderProgramComponent)
freeComponentImplDefi(DXRenderPassComponent)
#endif

freeComponentImplDefi(PhysicsDataComponent)

ObjectStatus InnoMemorySystem::getStatus()
{
	return InnoMemorySystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT void * InnoMemorySystem::allocateRawMemory(size_t size)
{
	auto m_Ptr = ::new char[size];
	MemoryWatchdog::get().recordRawMemoryUsage(m_Ptr, size);
	return m_Ptr;
}