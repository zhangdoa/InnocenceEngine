#include "../../Engine/Common/STL14.h"
#include "../../Engine/Common/STL17.h"

#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

std::string LoadShaderFile(const std::string& relativePath, const std::string& fileName)
{
	auto f_findIncludeFilePath = [](const std::string & content)
	{
		auto l_includePos = content.find("#include ");
		return l_includePos;
	};

	auto f_findGLSLExtensionPos = [](const std::string & content)
	{
		size_t l_glslExtensionPos = std::string::npos;

		l_glslExtensionPos = content.find(".glsl");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".vert");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".tesc");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".tese");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".geom");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".frag");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		l_glslExtensionPos = content.find(".comp");
		if (l_glslExtensionPos != std::string::npos)
		{
			return l_glslExtensionPos;
		}

		return l_glslExtensionPos;
	};

	std::ifstream l_file;

	auto l_workingDirectory = fs::current_path().generic_string();
	l_workingDirectory += "//";
	auto l_filePath = fs::path(l_workingDirectory + relativePath + fileName).generic_string();

	l_file.open(l_filePath.c_str(), std::ios::in);

	if (!l_file.is_open())
	{
		std::cerr << "File: " << fileName << " is not exist!" << std::endl;
		return std::string();
	}

	auto pbuf = l_file.rdbuf();
	std::size_t l_size = pbuf->pubseekoff(0, l_file.end, l_file.in);
	pbuf->pubseekpos(0, l_file.in);

	std::vector<char> l_buffer(l_size);
	pbuf->sgetn(&l_buffer[0], l_size);

	l_file.close();

	std::string l_content = &l_buffer[0];
	auto l_includePos = f_findIncludeFilePath(l_content);

	while (l_includePos != std::string::npos)
	{
		auto l_GLSLExtensionPos = f_findGLSLExtensionPos(l_content);
		auto l_includedFileName = l_content.substr(l_includePos + 10, l_GLSLExtensionPos - 5 - l_includePos);
		l_content.replace(l_includePos, l_GLSLExtensionPos - l_includePos + 6, LoadShaderFile(relativePath, l_includedFileName));

		l_includePos = f_findIncludeFilePath(l_content);
	}

	return l_content;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " INPUT" << " OUTPUT" << std::endl;
		return 0;
	}

	auto l_workingDirectory = fs::current_path().generic_string();
	l_workingDirectory += "//";
	std::cout << "Working directory: " << l_workingDirectory << std::endl;

	auto l_relativePath = fs::path(argv[1]).remove_filename().generic_string();
	auto l_fileName = fs::path(argv[1]).filename().generic_string();

	auto l_content = LoadShaderFile(l_relativePath, l_fileName);

	std::ofstream l_output;
	l_output.open(l_workingDirectory + argv[2]);
	l_output << l_content;
	l_output.close();

	return 0;
}