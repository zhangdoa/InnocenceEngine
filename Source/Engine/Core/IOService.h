#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoContainer.h"

namespace Inno
{
	namespace IOService
	{
		bool setupWorkingDirectory();

		std::vector<char> loadFile(const char* filePath, IOMode openMode);
		bool saveFile(const char* filePath, const std::vector<char>& content, IOMode saveMode);

		bool isFileExist(const char* filePath);
		std::string getFilePath(const char* filePath);
		std::string getFileExtension(const char* filePath);
		std::string getFileName(const char* filePath);
		std::string getWorkingDirectory();
		std::string validateFileName(const char* filePath);

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
		inline bool serializeVector(std::ostream& os, const Array<T>& vector)
		{
			return serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		}

		template<typename T>
		inline bool serializeVector(std::ostream& os, const std::vector<T>& vector)
		{
			return serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		}

		inline std::size_t getFileSize(std::istream& is)
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
			auto l_fileSize = getFileSize(is);
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
		inline bool deserializeVector(std::istream& is, std::streamoff startPos, std::size_t size, Array<T>& vector)
		{
			return deserialize(is, startPos, size, &vector[0]);
		}

		template<typename T>
		inline bool deserializeVector(std::istream& is, std::streamoff startPos, std::size_t size, std::vector<T>& vector)
		{
			return deserialize(is, startPos, size, &vector[0]);
		}

		template<typename T>
		inline bool deserializeVector(std::istream& is, Array<T>& vector)
		{
			auto l_fileSize = getFileSize(is);
			vector.reserve(l_fileSize / sizeof(T));
			return deserialize(is, &vector[0]);
		}

		template<typename T>
		inline bool deserializeVector(std::istream& is, std::vector<T>& vector)
		{
			auto l_fileSize = getFileSize(is);
			vector.resize(l_fileSize / sizeof(T));
			return deserialize(is, &vector[0]);
		}
	}
}