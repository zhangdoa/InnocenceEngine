#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoContainer.h"

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

	inline bool serialize(std::ostream& os, void* ptr, size_t size)
	{
		os.write((char*)ptr, size);
		return true;
	}

	template<typename T>
	inline bool serializeVector(std::ostream& os, const Array<T>& vector)
	{
		serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		return true;
	}

	template<typename T>
	inline bool serializeVector(std::ostream& os, const std::vector<T>& vector)
	{
		serialize(os, (void*)&vector[0], vector.size() * sizeof(T));
		return true;
	}

	inline bool deserialize(std::istream& is, void* ptr)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, is.end, is.in);
		pbuf->pubseekpos(0, is.in);
		pbuf->sgetn((char*)ptr, l_size);
		return true;
	}

	template<typename T>
	inline bool deserializeVector(std::istream& is, std::streamoff startPos, std::size_t size, Array<T>& vector)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		pbuf->pubseekpos(startPos, is.in);
		pbuf->sgetn((char*)&vector[0], size);
		return true;
	}

	template<typename T>
	inline bool deserializeVector(std::istream& is, std::streamoff startPos, std::size_t size, std::vector<T>& vector)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		pbuf->pubseekpos(startPos, is.in);
		pbuf->sgetn((char*)&vector[0], size);
		return true;
	}

	template<typename T>
	inline bool deserializeVector(std::istream& is, Array<T>& vector)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, is.end, is.in);
		vector.reserve(l_size / sizeof(T));
		pbuf->pubseekpos(0, is.in);
		pbuf->sgetn((char*)&vector[0], l_size);
		return true;
	}

	template<typename T>
	inline bool deserializeVector(std::istream& is, std::vector<T>& vector)
	{
		// get pointer to associated buffer object
		auto pbuf = is.rdbuf();
		// get file size using buffer's members
		std::size_t l_size = pbuf->pubseekoff(0, is.end, is.in);
		vector.resize(l_size / sizeof(T));
		pbuf->pubseekpos(0, is.in);
		pbuf->sgetn((char*)&vector[0], l_size);
		return true;
	}
}
