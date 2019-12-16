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

struct FileWriter
{
	std::ofstream os;
};

void getAccessSpecifier(CXCursor cursor, FileWriter* fileWriter)
{
	fileWriter->os << "----AccessSpecifier: ";

	auto accessSpecifier = clang_getCXXAccessSpecifier(cursor);

	switch (accessSpecifier)
	{
	case CX_CXXInvalidAccessSpecifier:
		break;
	case CX_CXXPublic:
		fileWriter->os << "Public";
		break;
	case CX_CXXProtected:
		fileWriter->os << "Protected";
		break;
	case CX_CXXPrivate:
		fileWriter->os << "Private";
		break;
	default:
		break;
	}

	fileWriter->os << std::endl;
}

void getType(CXCursor cursor, FileWriter* fileWriter)
{
	fileWriter->os << "----Type: ";

	auto type = clang_getCursorType(cursor);
	auto typeNameCStr = clang_getCString(clang_getTypeSpelling(type));
	fileWriter->os << typeNameCStr;

	fileWriter->os << std::endl;
}

CXChildVisitResult visitor(CXCursor cursor, CXCursor, CXClientData clientData)
{
	CXSourceRange range = clang_getCursorExtent(cursor);
	CXSourceLocation location = clang_getRangeStart(range);

	if (clang_Location_isFromMainFile(location))
	{
		auto fileWriter = static_cast<FileWriter*>(clientData);

		auto kind = clang_getCursorKind(cursor);

		auto cursorName = clang_getCursorDisplayName(cursor);
		auto cursorNameCStr = clang_getCString(cursorName);

		if (kind == CXCursorKind::CXCursor_StructDecl)
		{
			fileWriter->os << "Structure: " << cursorNameCStr << std::endl;
		}

		if (kind == CXCursorKind::CXCursor_ClassDecl)
		{
			fileWriter->os << "Class: " << cursorNameCStr << std::endl;
		}

		if (kind == CXCursorKind::CXCursor_EnumDecl)
		{
			fileWriter->os << "Enum: " << cursorNameCStr << std::endl;
		}

		if (kind == CXCursorKind::CXCursor_FieldDecl)
		{
			fileWriter->os << "--Field: " << cursorNameCStr << std::endl;

			getAccessSpecifier(cursor, fileWriter);
			getType(cursor, fileWriter);
		}

		if (kind == CXCursorKind::CXCursor_FunctionDecl)
		{
			getAccessSpecifier(cursor, fileWriter);
			auto l_returnType = clang_getCursorResultType(cursor);
			auto l_returnTypeNameCStr = clang_getCString(clang_getTypeSpelling(l_returnType));

			fileWriter->os << "--Function: " << l_returnTypeNameCStr << " " << cursorNameCStr << std::endl;
		}

		if (kind == CXCursorKind::CXCursor_Namespace)
		{
			fileWriter->os << "Namespace: " << cursorNameCStr << std::endl;
		}

		clang_disposeString(cursorName);
	}

	return CXChildVisit_Recurse;
}

void inclusionVisitor(CXFile included_file, CXSourceLocation* inclusion_stack, unsigned include_len, CXClientData client_data)
{
}

bool ParseContent(const std::string& fileName, FileWriter& fileWriter)
{
	char* args[] = { "--language=c++" };

	auto index = clang_createIndex(0, 0);

	auto translationUnit = clang_parseTranslationUnit(index, fileName.c_str(), args, 1, nullptr, 0, CXTranslationUnit_SkipFunctionBodies);

	clang_getInclusions(translationUnit, inclusionVisitor, nullptr);

	auto cursor = clang_getTranslationUnitCursor(translationUnit);

	clang_visitChildren(cursor, visitor, &fileWriter);

	clang_disposeTranslationUnit(translationUnit);

	clang_disposeIndex(index);

	return "";
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " INPUT" << std::endl;
		return 0;
	}

	auto l_workingDirectory = fs::current_path().generic_string();
	l_workingDirectory += "//";
	auto l_filePath = fs::path(argv[1]).generic_string();
	auto l_fileName = fs::path(argv[1]).filename().generic_string();

	l_filePath = l_workingDirectory + l_filePath;

	FileWriter l_output;

	l_output.os.open(l_workingDirectory + "..//Res//Intermediate//" + l_fileName + ".refl");

	ParseContent(l_filePath, l_output);

	l_output.os.close();

	return 0;
}