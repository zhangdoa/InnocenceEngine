#include "../../Engine/Common/STL14.h"
#include "../../Engine/Common/STL17.h"
#include <clang-c/Index.h>

#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

void getAccessSpecifier(CXCursor cursor)
{
	std::cout << "----AccessSpecifier: ";

	auto accessSpecifier = clang_getCXXAccessSpecifier(cursor);

	switch (accessSpecifier)
	{
	case CX_CXXInvalidAccessSpecifier:
		break;
	case CX_CXXPublic:
		std::cout << "Public";
		break;
	case CX_CXXProtected:
		std::cout << "Protected";
		break;
	case CX_CXXPrivate:
		std::cout << "Private";
		break;
	default:
		break;
	}

	std::cout << std::endl;
}

void getType(CXCursor cursor)
{
	std::cout << "----Type: ";

	auto type = clang_getCursorType(cursor);
	auto typeNameCStr = clang_getCString(clang_getTypeSpelling(type));
	std::cout << typeNameCStr;

	std::cout << std::endl;
}

CXChildVisitResult visitor(CXCursor cursor, CXCursor, CXClientData)
{
	auto kind = clang_getCursorKind(cursor);

	auto cursorName = clang_getCursorDisplayName(cursor);
	auto cursorNameCStr = clang_getCString(cursorName);

	if (kind == CXCursorKind::CXCursor_StructDecl)
	{
		std::cout << "Structure: " << cursorNameCStr << std::endl;
	}

	if (kind == CXCursorKind::CXCursor_ClassDecl)
	{
		std::cout << "Class: " << cursorNameCStr << std::endl;
	}

	if (kind == CXCursorKind::CXCursor_EnumDecl)
	{
		std::cout << "Enum: " << cursorNameCStr << std::endl;
	}

	if (kind == CXCursorKind::CXCursor_FieldDecl)
	{
		std::cout << "--Field: " << cursorNameCStr << std::endl;

		getAccessSpecifier(cursor);
		getType(cursor);
	}

	if (kind == CXCursorKind::CXCursor_FunctionDecl)
	{
		getAccessSpecifier(cursor);

		std::cout << "--Function: " << cursorNameCStr << std::endl;
	}

	clang_disposeString(cursorName);

	return CXChildVisit_Recurse;
}

void inclusionVisitor(CXFile included_file, CXSourceLocation* inclusion_stack, unsigned include_len, CXClientData client_data)
{
}

bool ParseContent(const std::string& fileName)
{
	char* args[] = { "--language=c++" };

	auto index = clang_createIndex(0, 0);

	auto translationUnit = clang_parseTranslationUnit(index, fileName.c_str(), args, 1, nullptr, 0, CXTranslationUnit_SkipFunctionBodies);

	clang_getInclusions(translationUnit, inclusionVisitor, nullptr);

	auto cursor = clang_getTranslationUnitCursor(translationUnit);

	clang_visitChildren(cursor, visitor, nullptr);

	clang_disposeTranslationUnit(translationUnit);

	clang_disposeIndex(index);

	return "";
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
	auto l_fileName = fs::path(argv[1]).generic_string();
	l_workingDirectory += l_fileName;

	auto l_content = ParseContent(l_workingDirectory);

	std::ofstream l_output;
	l_output.open(l_workingDirectory + argv[2]);
	l_output << l_content;
	l_output.close();

	return 0;
}