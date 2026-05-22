#pragma once
#include "STL14.h"
#include "Array.h"

namespace Inno
{
	enum class IOMode { Text, Binary };
	
	class IOService
	{
	public:
		bool SetupWorkingDirectory();

		Inno::Array<char> LoadFile(const char* filePath, IOMode openMode);
		bool SaveFile(const char* filePath, const Inno::Array<char>& content, IOMode saveMode);

		bool IsFileExist(const char* filePath);
		std::string GetFilePath(const char* filePath);
		std::string GetFileExtension(const char* filePath);
		std::string GetFileName(const char* filePath);
		std::string GetWorkingDirectory();
		std::string GetDataDirectory();
		std::string GetEngineDirectory();
		std::string GetProjectName();
		std::string GetProjectDirectory();
		std::string GetGeneratedDirectory();
		std::string GetComponentDirectory();
		std::string ValidateFileName(const char* filePath);

		inline bool serialize(std::ostream& os, void* ptr, size_t size)
		{
			os.write((char*)ptr, size);
			return true;
		}

		template<typename T>
		inline bool serialize(std::ostream& os, const T* ptr)
		{
			return serialize(os, (void*)ptr, sizeof(T));
		}

		template<typename T>
		inline bool SerializeVector(std::ostream& os, const Inno::Array<T>& vector)
		{
			return serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		}

		inline std::size_t GetFileSize(std::istream& is)
		{
			auto pbuf = is.rdbuf();
			std::size_t l_size = pbuf->pubseekoff(0, is.end, is.in);
			pbuf->pubseekpos(0, is.in);
			return l_size;
		}

		inline bool deserialize(std::istream& is, std::streamoff startPos, std::size_t size, void* ptr)
		{
			auto pbuf = is.rdbuf();
			pbuf->pubseekpos(startPos, is.in);
			pbuf->sgetn((char*)ptr, size);
			return true;
		}

		inline bool deserialize(std::istream& is, void* ptr)
		{
			auto l_fileSize = GetFileSize(is);
			return deserialize(is, 0, l_fileSize, ptr);
		}

		template<typename T>
		inline bool deserialize(std::istream& is, T* ptr)
		{
			return deserialize(is, (void*)ptr);
		}

		template<typename T>
		inline bool deserialize(std::istream& is, std::streamoff startPos, T* ptr)
		{
			return deserialize(is, startPos, sizeof(T), (void*)ptr);
		}

		template<typename T>
		inline bool DeserializeVector(std::istream& is, std::streamoff startPos, std::size_t size, Inno::Array<T>& vector)
		{
			return deserialize(is, startPos, size, &vector[0]);
		}

		template<typename T>
		inline bool DeserializeVector(std::istream& is, Inno::Array<T>& vector)
		{
			auto l_fileSize = GetFileSize(is);
			vector.resize(l_fileSize / sizeof(T));
			return deserialize(is, &vector[0]);
		}

		struct CPPClassDesc
		{
			bool isInterface = false;
			bool isNonCopyable = true;
			bool isNonMoveable = true;
			std::string parentClass;
			std::string className;
			std::string filePath;
		};
		
		bool AddCPPClassFiles(const CPPClassDesc& desc);

	private:
		std::string m_workingDir;
		std::string m_dataDir;
	};
}