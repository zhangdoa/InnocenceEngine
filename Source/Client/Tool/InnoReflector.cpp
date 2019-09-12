#include "../../Engine/Common/STL14.h"
#include "../../Engine/Common/STL17.h"

#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

enum class ReflTypeInfo
{
	refl_none,
	refl_struct,
	refl_class,
	refl_union,
	refl_enum,
	refl_char,
	refl_bool,
	refl_int,
	refl_uint,
	refl_float,
	refl_double,
};

struct ReflInfo
{
	const char* m_name = 0;
	bool isPtr = false;
	ReflTypeInfo m_type = ReflTypeInfo::refl_none;
};

std::string GetName(std::string& content, const char* keyword, const char* endChar)
{
	auto l_keywordPosBegin = content.find(keyword);
	auto l_keywordPosEnd = content.find(endChar);
	auto l_splitPosBegin = l_keywordPosBegin + strlen(keyword) + 1;
	auto l_splitOffset = l_keywordPosEnd - l_splitPosBegin;

	auto l_name = content.substr(l_splitPosBegin, l_splitOffset);

	return l_name;
}

void ParseMemberDecl(std::stringstream& ss, std::string& line, const char* keyword, const char* endChar)
{
	auto l_keywordPosBegin = line.find(keyword);
	if (l_keywordPosBegin != std::string::npos)
	{
		const char* l_isPtr;
		auto l_name = GetName(line, keyword, endChar);

		auto l_derefChar = line.substr(l_keywordPosBegin + strlen(keyword), 1);

		if (l_derefChar == "*")
		{
			l_isPtr = "true";
			l_name = l_name.substr(1, std::string::npos);
		}
		else
		{
			l_isPtr = "false";
		}

		ss << "\tstatic ReflInfo refl_";
		ss << l_name;
		ss << " = { \"";
		ss << l_name;
		ss << "\", ";
		ss << l_isPtr;
		ss << ", ReflTypeInfo::refl_";
		ss << keyword;
		ss << " };";
		ss << std::endl;
	}
}

void ParseDecl(std::stringstream& ss, std::string& line, const char* keyword, const char* endChar)
{
	auto l_keywordPosBegin = line.find(keyword);
	if (l_keywordPosBegin != std::string::npos)
	{
		auto l_name = GetName(line, keyword, endChar);

		auto l_hasParent = l_name.find(" : ");

		if (l_hasParent != std::string::npos)
		{
			l_name = l_name.substr(0, l_hasParent);
		}

		ss << "struct " << "Refl_" << l_name << std::endl;
		ss << "{" << std::endl;

		ss << "\tstatic ReflInfo refl_";
		ss << l_name;
		ss << " = { \"";
		ss << l_name;
		ss << "\", ";
		ss << "false";
		ss << ", ReflTypeInfo::refl_";
		ss << keyword;
		ss << " };";
		ss << std::endl;
	}
}

std::string ParseContent(const std::string& fileName)
{
	std::ifstream l_file;

	auto l_workingDirectory = fs::current_path().generic_string();
	l_workingDirectory += "//";
	l_workingDirectory += fileName;

	auto l_filePath = fs::path(l_workingDirectory).generic_string();

	l_file.open(l_filePath.c_str(), std::ios::in);

	if (!l_file.is_open())
	{
		std::cerr << "File: " << fileName << " is not exist!" << std::endl;
		return std::string();
	}

	std::stringstream ss;

	std::string l_line;
	while (std::getline(l_file, l_line))
	{
		if (l_line == "};")
		{
			ss << "};" << std::endl;
			ss << std::endl;
		}
		else
		{
			l_line.append("\n");
			ParseDecl(ss, l_line, "struct", "\n");
			ParseDecl(ss, l_line, "class", "\n");
			ParseDecl(ss, l_line, "union", "\n");

			ParseMemberDecl(ss, l_line, "char", " = ");
			ParseMemberDecl(ss, l_line, "bool", " = ");
			ParseMemberDecl(ss, l_line, "int", " = ");
			ParseMemberDecl(ss, l_line, "float", " = ");
			ParseMemberDecl(ss, l_line, "double", " = ");
		}
	}

	return ss.str();
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

	auto l_fileName = fs::path(argv[1]).filename().generic_string();

	auto l_content = ParseContent(l_fileName);

	std::ofstream l_output;
	l_output.open(l_workingDirectory + argv[2]);
	l_output << l_content;
	l_output.close();

	return 0;
}