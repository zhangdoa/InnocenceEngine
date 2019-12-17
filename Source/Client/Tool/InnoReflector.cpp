#include "../../Engine/Common/STL14.h"
#include "../../Engine/Common/STL17.h"
#include "../../Engine/Common/InnoMetadata.h"
#include <clang-c/Index.h>

#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace InnoReflector
{
	struct FileWriter
	{
		std::ofstream os;
	};

	struct ClangMetadata
	{
		const char* name;
		CXCursorKind cursorKind;
		CX_CXXAccessSpecifier accessSpecifier;
		CXTypeKind typeKind;
		CXTypeKind returnTypeKind;
		size_t arraySize = 0;
	};

	std::vector<ClangMetadata> m_clangMetadata;

	CXChildVisitResult visitor(CXCursor cursor, CXCursor, CXClientData clientData)
	{
		CXSourceRange range = clang_getCursorExtent(cursor);
		CXSourceLocation location = clang_getRangeStart(range);
		ClangMetadata l_metadata;

		if (clang_Location_isFromMainFile(location))
		{
			auto kind = clang_getCursorKind(cursor);

			if (clang_isDeclaration(kind))
			{
				auto cursorName = clang_getCursorDisplayName(cursor);
				auto cursorNameCStr = clang_getCString(cursorName);

				l_metadata.name = cursorNameCStr;
				l_metadata.cursorKind = kind;
				l_metadata.accessSpecifier = clang_getCXXAccessSpecifier(cursor);
				auto l_type = clang_getCursorType(cursor);

				if (l_type.kind == CXTypeKind::CXType_ConstantArray)
				{
					l_metadata.arraySize = clang_getArraySize(l_type);
					l_type = clang_getArrayElementType(l_type);
					l_metadata.typeKind = l_type.kind;
				}

				l_metadata.typeKind = l_type.kind;

				auto l_returnType = clang_getCursorResultType(cursor);

				l_metadata.returnTypeKind = l_returnType.kind;

				m_clangMetadata.emplace_back(l_metadata);

				//clang_disposeString(cursorName);
			}
		}

		return CXChildVisit_Recurse;
	}

	void inclusionVisitor(CXFile included_file, CXSourceLocation* inclusion_stack, unsigned include_len, CXClientData client_data)
	{
	}

	void writeCursorKind(CXCursorKind typeKind, FileWriter* fileWriter)
	{
		switch (typeKind)
		{
		case CXCursor_UnexposedDecl:
			fileWriter->os << "DeclType::Unexposed";
			break;
		case CXCursor_StructDecl:
			fileWriter->os << "DeclType::Struct";
			break;
		case CXCursor_UnionDecl:
			fileWriter->os << "DeclType::Union";
			break;
		case CXCursor_ClassDecl:
			fileWriter->os << "DeclType::Class";
			break;
		case CXCursor_EnumDecl:
			fileWriter->os << "DeclType::Enum";
			break;
		case CXCursor_FieldDecl:
			fileWriter->os << "DeclType::Var";
			break;
		case CXCursor_ParmDecl:
			fileWriter->os << "DeclType::Parm";
			break;
		case CXCursor_CXXMethod:
			fileWriter->os << "DeclType::Function";
			break;
		case CXCursor_Namespace:
			fileWriter->os << "DeclType::Namespace";
			break;
		case CXCursor_Constructor:
			fileWriter->os << "DeclType::Ctor";
			break;
		case CXCursor_Destructor:
			fileWriter->os << "DeclType::Dtor";
			break;
		case CXCursor_DLLExport:
			break;
		case CXCursor_DLLImport:
			break;
		default:
			break;
		}
	}

	void writeAccessSpecifier(CX_CXXAccessSpecifier accessSpecifier, FileWriter* fileWriter)
	{
		fileWriter->os << "AccessType::";

		switch (accessSpecifier)
		{
		case CX_CXXInvalidAccessSpecifier:
			fileWriter->os << "Invalid";
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
	}

	void writeTypeKind(CXTypeKind typeKind, FileWriter* fileWriter)
	{
		auto typeKindNameCStr = clang_getCString(clang_getTypeKindSpelling(typeKind));
		fileWriter->os << typeKindNameCStr;
	}

	void writeFile(FileWriter* fileWriter)
	{
		fileWriter->os << "#pragma once" << std::endl;
		fileWriter->os << "#include \"../../Source/Engine/Common/InnoMetadata.h\"" << std::endl;
		fileWriter->os << "using namespace InnoMetadata;" << std::endl;
		fileWriter->os << std::endl;

		for (auto i : m_clangMetadata)
		{
			if (i.cursorKind != CXCursorKind::CXCursor_CXXMethod && i.cursorKind != CXCursorKind::CXCursor_ParmDecl)
			{
				fileWriter->os << "Metadata ";
				fileWriter->os << "refl_" << i.name << " = { \"" << i.name << "\", ";
				writeCursorKind(i.cursorKind, fileWriter);
				fileWriter->os << ", ";
				writeAccessSpecifier(i.accessSpecifier, fileWriter);
				//fileWriter->os << ", ";
				//writeTypeKind(i.typeKind, fileWriter);
				//fileWriter->os << ", ";
				//fileWriter->os << i.arraySize << ", ";
				//if (i.cursorKind == CXCursorKind::CXCursor_CXXMethod || i.cursorKind == CXCursorKind::CXCursor_ParmDecl)
				//{
				//	writeTypeKind(i.returnTypeKind, fileWriter);
				//}
				//else
				//{
				//	fileWriter->os << "Invalid";
				//}

				fileWriter->os << " };" << std::endl;

				fileWriter->os << std::endl;

				//fileWriter->os << "template<>" << std::endl;
				//fileWriter->os << "Metadata GetMetadata(const " << i.name << "& rhs)" << std::endl;
				//fileWriter->os << "{" << std::endl;
				//fileWriter->os << "  return refl_" << i.name << ";" << std::endl;
				//fileWriter->os << "}" << std::endl;

				//fileWriter->os << std::endl;
			}
		}
	}

	void parseContent(const std::string& fileName, FileWriter& fileWriter)
	{
		char* args[] = { "--language=c++" };

		auto index = clang_createIndex(0, 0);

		auto translationUnit = clang_parseTranslationUnit(index, fileName.c_str(), args, 1, nullptr, 0, CXTranslationUnit_SkipFunctionBodies);
		//auto l_resUsages = clang_getCXTUResourceUsage(translationUnit);

		// @TODO: Reserve with a meaningful size
		m_clangMetadata.reserve(8192);

		auto cursor = clang_getTranslationUnitCursor(translationUnit);

		clang_visitChildren(cursor, visitor, nullptr);

		m_clangMetadata.shrink_to_fit();

		writeFile(&fileWriter);

		clang_disposeTranslationUnit(translationUnit);

		clang_disposeIndex(index);
	}
}

using namespace InnoReflector;

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " INPUT" << std::endl;
		return 0;
	}

	auto l_workingDirectory = fs::current_path().generic_string();
	l_workingDirectory += "//";
	auto l_filePath = fs::path(argv[1]);
	auto l_filePathStr = l_filePath.generic_string();
	auto l_fileName = l_filePath.stem().generic_string();

	l_filePathStr = l_workingDirectory + l_filePathStr;

	FileWriter l_output;

	l_output.os.open(l_workingDirectory + "..//Res//Intermediate//" + l_fileName + ".refl.h");

	parseContent(l_filePathStr, l_output);

	l_output.os.close();

	return 0;
}