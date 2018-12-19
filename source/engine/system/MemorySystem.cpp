#include "MemorySystem.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

template <class T>
class ComponentPool
{
public:
	ComponentPool() : ComponentPool(16384) {
	};
	ComponentPool(unsigned long long poolTotalCapability) {
		m_poolTotalCapability = poolTotalCapability;
		m_poolCurrentFreeBlock = poolTotalCapability;
		m_poolSize = m_poolTotalCapability * sizeof(T);
		m_poolPtr = ::new unsigned char[m_poolSize];
		m_poolFreePtr = m_poolPtr;
	};
	~ComponentPool() {
		::delete[] m_poolPtr;
	};

	unsigned long long m_poolTotalCapability = 0;
	unsigned long long m_poolCurrentFreeBlock = 0;
	unsigned long long m_poolSize = 0;
	unsigned char* m_poolPtr = nullptr;
	unsigned char* m_poolFreePtr = nullptr;
};

//Memory pool for others
class ChuckPool
{
public:
	ChuckPool() : ChuckPool(1024 * 1024 * 64) {
	};
	ChuckPool(unsigned long long poolSize) {
		m_poolSize = poolSize;
		m_availablePoolSize = poolSize;
		m_PoolPtr = ::new unsigned char[m_poolSize];
		m_PoolFreePtr = m_PoolPtr;
	};
	~ChuckPool() {
		::delete[] m_PoolPtr;
	};

	unsigned long long  m_poolSize = 0;
	unsigned long long  m_availablePoolSize = 0;
	unsigned char* m_PoolPtr = nullptr;
	unsigned char* m_PoolFreePtr = nullptr;
};

INNO_PRIVATE_SCOPE InnoMemorySystemNS
{
#define componentPoolUniPtr( className ) \
std::unique_ptr<ComponentPool<className>> m_##className##Pool;

	//Memory pool for components
	componentPoolUniPtr(TransformComponent);
	componentPoolUniPtr(VisibleComponent);
	componentPoolUniPtr(DirectionalLightComponent);
	componentPoolUniPtr(PointLightComponent);
	componentPoolUniPtr(SphereLightComponent);
	componentPoolUniPtr(CameraComponent);
	componentPoolUniPtr(InputComponent);
	componentPoolUniPtr(EnvironmentCaptureComponent);

	componentPoolUniPtr(MeshDataComponent);
	componentPoolUniPtr(MaterialDataComponent);
	componentPoolUniPtr(TextureDataComponent);

	componentPoolUniPtr(GLMeshDataComponent);
	componentPoolUniPtr(GLTextureDataComponent);
	componentPoolUniPtr(GLFrameBufferComponent);
	componentPoolUniPtr(GLShaderProgramComponent);
	componentPoolUniPtr(GLRenderPassComponent);

	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	componentPoolUniPtr(DXMeshDataComponent);
	componentPoolUniPtr(DXTextureDataComponent);
	componentPoolUniPtr(DXShaderProgramComponent);
	componentPoolUniPtr(DXRenderPassComponent);
	#endif

	componentPoolUniPtr(PhysicsDataComponent);

	//Memory pool for vertices and indices
	// @TODO: need more efficient solution
	std::unique_ptr<ComponentPool<Vertex>> m_VertexPool;
	std::unique_ptr<ComponentPool<Index>> m_IndexPool;

	static const uint32_t s_BlockSizes[] = {
		// 4-increments
		4,  8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48,
		52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96,

		// 32-increments
		128, 160, 192, 224, 256, 288, 320, 352, 384,
		416, 448, 480, 512, 544, 576, 608, 640,

		// 64-increments
		704, 768, 832, 896, 960, 1024
	};

	static const uint32_t s_NumBlockSizes = sizeof(s_BlockSizes) / sizeof(s_BlockSizes[0]);
	static const uint32_t s_minBlockSize = s_BlockSizes[0];
	static const uint32_t s_MaxBlockSize = s_BlockSizes[s_NumBlockSizes - 1];

	std::unique_ptr<ChuckPool> m_ChuckPool;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	void createChuck();
}

//Double-linked-list
class Chunk
{
public:
	Chunk(unsigned int chuckSize) : m_next(nullptr),
		m_prev(nullptr),
		m_blockSize(chuckSize),
		m_free(true) {};

	Chunk* m_next;
	Chunk* m_prev;
	unsigned int m_blockSize;
	bool m_free;
};

INNO_SYSTEM_EXPORT bool InnoMemorySystem::setup()
{
#define constructComponentPool( className ) \
	InnoMemorySystemNS::m_##className##Pool = std::make_unique<ComponentPool<className>>();

	constructComponentPool(TransformComponent);
	constructComponentPool(VisibleComponent);
	constructComponentPool(DirectionalLightComponent);
	constructComponentPool(PointLightComponent);
	constructComponentPool(SphereLightComponent);
	constructComponentPool(CameraComponent);
	constructComponentPool(InputComponent);
	constructComponentPool(EnvironmentCaptureComponent);

	constructComponentPool(MeshDataComponent);
	constructComponentPool(MaterialDataComponent);
	constructComponentPool(TextureDataComponent);

	constructComponentPool(GLMeshDataComponent);
	constructComponentPool(GLTextureDataComponent);
	constructComponentPool(GLFrameBufferComponent);
	constructComponentPool(GLShaderProgramComponent);
	constructComponentPool(GLRenderPassComponent);

	#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
	constructComponentPool(DXMeshDataComponent);
	constructComponentPool(DXTextureDataComponent);
	constructComponentPool(DXShaderProgramComponent);
	constructComponentPool(DXRenderPassComponent);
	#endif

	constructComponentPool(PhysicsDataComponent);

	InnoMemorySystemNS::m_ChuckPool = std::make_unique<ChuckPool>();
	// fill the chuck pool with empty marker
	std::memset(InnoMemorySystemNS::m_ChuckPool->m_PoolPtr, 0xCC, InnoMemorySystemNS::m_ChuckPool->m_poolSize);

	// first free chuck
	InnoMemorySystemNS::createChuck();

	return true;
}

void InnoMemorySystemNS::createChuck()
{
	Chunk l_freeChunk(InnoMemorySystemNS::m_ChuckPool->m_poolSize);
	std::memcpy(InnoMemorySystemNS::m_ChuckPool->m_PoolPtr, &l_freeChunk, sizeof(Chunk));
	InnoMemorySystemNS::m_ChuckPool->m_availablePoolSize = InnoMemorySystemNS::m_ChuckPool->m_poolSize - sizeof(Chunk);
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

INNO_SYSTEM_EXPORT void * InnoMemorySystem::allocate(unsigned long size)
{
	// add bound check size
	// [Chuck + data]
	unsigned long l_requiredSize = size + sizeof(Chunk);
	// alignment to 4
	// [Chuck + data + alignment]
	for (size_t i = 0; i < InnoMemorySystemNS::s_NumBlockSizes; i++)
	{
		if (l_requiredSize <= InnoMemorySystemNS::s_BlockSizes[i])
		{
			l_requiredSize = InnoMemorySystemNS::s_BlockSizes[i];
			break;
		}
	}
	// Now search for a block big enough from the beginning of the pool, double linked list, O(n)
	//[suitable block] is larger than
	//[Chuck + data + alignment]
	Chunk* l_suitableChuckPtr = reinterpret_cast<Chunk*>(InnoMemorySystemNS::m_ChuckPool->m_PoolFreePtr);
	while (l_suitableChuckPtr)
	{
		if (l_suitableChuckPtr->m_free && l_suitableChuckPtr->m_blockSize >= l_requiredSize) { break; }
		l_suitableChuckPtr = l_suitableChuckPtr->m_next;
	}

	// If no block is found, return nullptr
	if (!l_suitableChuckPtr)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Can't allocate memory!");
		return nullptr;
	}

	// If the block is valid, create a new free block with
	// what remains of the block memory
	unsigned long l_freeBlockSize = l_suitableChuckPtr->m_blockSize - l_requiredSize;
	//[      suitable block       ] is divided to
	//[required block + free block]
	unsigned char* l_suitableChuckPtr_UC = reinterpret_cast<unsigned char*>(l_suitableChuckPtr);
	Chunk* l_freeChuckPtr = reinterpret_cast<Chunk*>(l_suitableChuckPtr_UC + l_requiredSize);
	Chunk l_freeChuck(l_freeBlockSize);

	// assign the next block's prev pointer to this free block
	l_freeChuck.m_prev = l_suitableChuckPtr;
	l_freeChuck.m_next = l_suitableChuckPtr->m_next;
	if (l_freeChuck.m_next)
	{
		l_freeChuck.m_next->m_prev = l_freeChuckPtr;
	}

	// set allocated block chuck data
	l_suitableChuckPtr->m_next = l_freeChuckPtr;
	l_suitableChuckPtr->m_blockSize = l_requiredSize;
	l_suitableChuckPtr->m_free = false;

	// Fill the block with filling flags
	std::memset(l_suitableChuckPtr_UC + sizeof(Chunk), 0xAB, size);

	unsigned char* l_freeChuckPtr_UC = reinterpret_cast<unsigned char*>(l_freeChuckPtr);
	std::memcpy(l_freeChuckPtr_UC, &l_freeChuck, sizeof(Chunk));

	// update the pool size
	InnoMemorySystemNS::m_ChuckPool->m_availablePoolSize -= l_requiredSize;

	return (l_suitableChuckPtr_UC + sizeof(Chunk));
}

INNO_SYSTEM_EXPORT void InnoMemorySystem::free(void * ptr)
{
	// is a valid node?
	if (!ptr) return;

	// already free?
	Chunk* l_chuckPtr = reinterpret_cast<Chunk*>((unsigned char*)ptr - sizeof(Chunk));
	if (l_chuckPtr->m_free) return;

	unsigned long l_fullFreeBlockSize = l_chuckPtr->m_blockSize;
	InnoMemorySystemNS::m_ChuckPool->m_availablePoolSize += l_fullFreeBlockSize;

	Chunk* l_freeChuckPtr = l_chuckPtr;
	Chunk* l_prevChuckPtr = l_chuckPtr->m_prev;
	Chunk* l_nextChuckPtr = l_chuckPtr->m_next;

	if (l_prevChuckPtr)
	{
		// If the node before is free merge it with this one
		if (l_prevChuckPtr->m_free)
		{
			l_freeChuckPtr = l_prevChuckPtr;
			if (l_freeChuckPtr->m_prev)
			{
				l_prevChuckPtr = l_freeChuckPtr->m_prev;
			}
			else
			{
				l_prevChuckPtr = nullptr;
			}

			// Include the prev node in the block size so we trash it as well
			l_fullFreeBlockSize += l_prevChuckPtr->m_blockSize;
		}
	}
	// If there is a next one, we need to update its pointer
	if (l_nextChuckPtr)
	{
		// We will re point the next of the freer's prev
		l_nextChuckPtr->m_prev = l_freeChuckPtr;

		// Include the next node in the block size if it is
		// free so we trash it as well
		if (l_nextChuckPtr->m_free)
		{
			l_fullFreeBlockSize += l_nextChuckPtr->m_blockSize;

			// We will point to next's next
			if (l_nextChuckPtr->m_next)
			{
				l_nextChuckPtr = l_nextChuckPtr->m_next;
				l_nextChuckPtr->m_prev = l_freeChuckPtr;
			}
			else
			{
				l_nextChuckPtr = nullptr;
			}
		}
	}
	// Create the free block
	unsigned char* l_freeChuckPtr_UC = reinterpret_cast<unsigned char*>(l_freeChuckPtr);

	Chunk l_freeChuck(l_fullFreeBlockSize);
	l_freeChuck.m_prev = l_prevChuckPtr;
	l_freeChuck.m_next = l_nextChuckPtr;
	std::memcpy(l_freeChuckPtr_UC, &l_freeChuck, sizeof(Chunk));
	std::memset(l_freeChuckPtr_UC + sizeof(Chunk), 0xCC, l_fullFreeBlockSize - sizeof(Chunk));
}

void InnoMemorySystem::serializeImpl(const std::string& fileName, const std::string& className, unsigned long classSize, void* ptr)
{
	if (!ptr) return;
	char* l_ptr_UC = reinterpret_cast<char*>(ptr);

	std::ofstream l_file;
	// @TODO: just return the streaming data, leave IO for AssetSystem
	auto l_metaInfo = std::string("[DataType:InnocenceEngineSerializationData]");
	auto l_time = "[Timestamp:" + std::to_string(g_pCoreSystem->getTimeSystem()->getCurrentTime()) + "]";
	auto l_classNameChr = "[ClassName:" + className + "]";

	auto x = l_metaInfo.length();
	l_file.open(fileName + ".innoAsset", std::ios::out | std::ios::trunc | std::ios::binary);
	l_file.write(l_metaInfo.c_str(), l_metaInfo.length());
	l_file.write(l_time.c_str(), l_time.length());
	l_file.write(l_classNameChr.c_str(), l_classNameChr.length());
	l_file.write(l_ptr_UC, classSize);
	l_file.close();
}

void* InnoMemorySystem::deserializeImpl(const std::string& fileName)
{
	std::ifstream l_file;
	l_file.open(fileName, std::ios::binary);

	if (!l_file.is_open())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Can't open file " + fileName + " for deserialization!");
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
	unsigned char* buffer_UC = reinterpret_cast<unsigned char*>(buffer);

	std::string className;

	size_t i = 0;
	size_t j = 1;

	for (i; i < l_size; i++)
	{
		if (buffer_UC[i] == *"[")
		{
			if (buffer_UC[i + 1] == *"C")
			{
				while (buffer_UC[i + j] != *"]")
				{
					className.push_back(buffer_UC[i + j]);
					j++;
				}
				break;
			}
		}
	}

	className = className.substr(className.find(":") + 1, std::string::npos);

	unsigned long classSize;
	unsigned char* l_ptr;

	if (className == getClassName<TransformComponent>())
	{
		classSize = sizeof(TransformComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<TransformComponent>());
	}
	else if (className == getClassName<VisibleComponent>())
	{
		classSize = sizeof(VisibleComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<VisibleComponent>());
	}
	else if (className == getClassName<DirectionalLightComponent>())
	{
		classSize = sizeof(DirectionalLightComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<DirectionalLightComponent>());
	}
	else if (className == getClassName<PointLightComponent>())
	{
		classSize = sizeof(PointLightComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<PointLightComponent>());
	}
	else if (className == getClassName<SphereLightComponent>())
	{
		classSize = sizeof(SphereLightComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<SphereLightComponent>());
	}
	else if (className == getClassName<CameraComponent>())
	{
		classSize = sizeof(CameraComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<CameraComponent>());
	}
	else if (className == getClassName<InputComponent>())
	{
		classSize = sizeof(InputComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<InputComponent>());
	}
	else if (className == getClassName<EnvironmentCaptureComponent>())
	{
		classSize = sizeof(EnvironmentCaptureComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<EnvironmentCaptureComponent>());
	}
	else if (className == getClassName<PhysicsDataComponent>())
	{
		classSize = sizeof(PhysicsDataComponent);
		l_ptr = reinterpret_cast<unsigned char*>(spawn<PhysicsDataComponent>());
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Unsupported deserialization data type "+ className + " !");
		return nullptr;
	}

	std::memcpy(l_ptr, buffer_UC + i + j + 1, classSize);

	l_file.close();

	return l_ptr;
}

INNO_SYSTEM_EXPORT void InnoMemorySystem::dumpToFile(bool fullDump)
{
	std::ofstream l_file;
	auto l_timeData = g_pCoreSystem->getTimeSystem()->getCurrentTimeInLocal();
	auto l_timeString =
		std::to_string(l_timeData.year)
		+ "-" + std::to_string(l_timeData.month)
		+ "-" + std::to_string(l_timeData.day)
		+ "-" + std::to_string(l_timeData.hour)
		+ "-" + std::to_string(l_timeData.minute)
		+ "-" + std::to_string(l_timeData.second)
		+ "-" + std::to_string(l_timeData.millisecond);
	l_file.open("../" + l_timeString + ".innoMemoryDump");

	l_file << "InnoMemory Pool Dump File ----------------------------------\n";
	l_file << "Total Size: " << InnoMemorySystemNS::m_ChuckPool->m_poolSize << "\n";
	l_file << "Free Size: " << InnoMemorySystemNS::m_ChuckPool->m_availablePoolSize << "\n";

	Chunk* block = reinterpret_cast<Chunk*>(InnoMemorySystemNS::m_ChuckPool->m_PoolPtr);

	while (block)
	{
		if (block->m_free)
		{
			l_file << "Free: " << block << "[Bytes: " << block->m_blockSize << "]\n";
		}
		else
		{
			l_file << "Used: " << block << "[Bytes: " << block->m_blockSize << "]\n";
		}
		block = block->m_next;
	}

	if (fullDump)
	{
		l_file << "\n\nMemory Dump:\n";
		unsigned char* ptr = InnoMemorySystemNS::m_ChuckPool->m_PoolPtr;
		unsigned char* charPtr = InnoMemorySystemNS::m_ChuckPool->m_PoolPtr;

		l_file << "Start: " << ptr << "\n";
		unsigned char i = 0;

		// Write the hex memory data
		unsigned long long bytesPerLine = 32;

		l_file << "\n" << ptr << ": " << *(ptr);
		++ptr;
		for (i = 1; ((unsigned long long)(ptr - InnoMemorySystemNS::m_ChuckPool->m_PoolPtr) < InnoMemorySystemNS::m_ChuckPool->m_poolSize); ++i, ++ptr)
		{
			if (i == bytesPerLine)
			{
				// Write all the chars for this line now
				l_file << "  " << charPtr;
				for (unsigned long long charI = 0; charI < bytesPerLine; ++charI, ++charPtr)
					l_file << *charPtr;
				charPtr = ptr;

				// Write the new line memory data
				l_file << "\n" << ptr << ": " << *(ptr);
				i = 0;
			}
			else
				l_file << *(ptr);
		}

		// Fill any gaps in the tab
		if ((unsigned long long)(ptr - InnoMemorySystemNS::m_ChuckPool->m_PoolPtr) >= InnoMemorySystemNS::m_ChuckPool->m_poolSize)
		{
			unsigned long long lastLineBytes = i;
			for (i; i < bytesPerLine; i++)
				l_file << " --";

			// Write all the chars for this line now
			l_file << "  " << charPtr;
			for (unsigned long long charI = 0; charI < lastLineBytes; ++charI, ++charPtr)
				l_file << *charPtr;
			charPtr = ptr;
		}
	}

	l_file.close();
}

#define allocateComponentImplDefi( className ) \
className* InnoMemorySystem::allocate##className() \
{ \
	auto l_ptr = new(InnoMemorySystemNS::m_##className##Pool->m_poolFreePtr) className(); \
	if (l_ptr) \
	{ \
		InnoMemorySystemNS::m_##className##Pool->m_poolFreePtr += sizeof(className); \
		--InnoMemorySystemNS::m_##className##Pool->m_poolCurrentFreeBlock; \
		return l_ptr; \
	} \
	else \
	{ \
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "MemorySystem: Can't allocate memory for " + std::string(#className) + " !"); \
		return nullptr; \
	} \
} \

allocateComponentImplDefi(TransformComponent)
allocateComponentImplDefi(VisibleComponent)
allocateComponentImplDefi(DirectionalLightComponent);
allocateComponentImplDefi(PointLightComponent);
allocateComponentImplDefi(SphereLightComponent);
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
void InnoMemorySystem::free##className(className* p) \
{ \
} \

freeComponentImplDefi(TransformComponent)
freeComponentImplDefi(VisibleComponent)
freeComponentImplDefi(DirectionalLightComponent);
freeComponentImplDefi(PointLightComponent);
freeComponentImplDefi(SphereLightComponent);
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

freeComponentImplDefi(PhysicsDataComponent);

ObjectStatus InnoMemorySystem::getStatus()
{
	return InnoMemorySystemNS::m_objectStatus;
}
