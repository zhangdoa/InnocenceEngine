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
		const char* typeName;
		CXTypeKind returnTypeKind;
		const char* returnTypeName;
		size_t arraySize = 0;
		size_t totalChildrenCount = 0;
		size_t validChildrenCount = 0;
	};

	std::vector<ClangMetadata> m_clangMetadata;

	CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData clientData)
	{
		CXSourceRange range = clang_getCursorExtent(cursor);
		CXSourceLocation location = clang_getRangeStart(range);
		ClangMetadata l_metadata;

		if (clang_Location_isFromMainFile(location))
		{
			auto kind = clang_getCursorKind(cursor);

			if (clang_isDeclaration(kind) && kind != CXCursorKind::CXCursor_CXXAccessSpecifier)
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
				}

				l_metadata.typeKind = l_type.kind;

				auto typeName = clang_getTypeSpelling(l_type);
				auto typeNameCStr = clang_getCString(typeName);
				l_metadata.typeName = typeNameCStr;

				auto l_returnType = clang_getCursorResultType(cursor);

				l_metadata.returnTypeKind = l_returnType.kind;
				auto returnTypeName = clang_getTypeSpelling(l_returnType);
				auto returnTypeNameCStr = clang_getCString(returnTypeName);
				l_metadata.returnTypeName = returnTypeNameCStr;

				//auto l_semanticParent = clang_getCursorSemanticParent(cursor);
				auto l_semanticParent = parent;
				auto l_semanticParentName = clang_getCursorDisplayName(l_semanticParent);
				auto l_semanticParentNameCStr = clang_getCString(l_semanticParentName);

				auto l_parent = std::find_if(m_clangMetadata.begin(), m_clangMetadata.end(),
					[&](ClangMetadata& parent)
				{
					return !strcmp(parent.typeName, l_semanticParentNameCStr);
				});

				if (l_parent != m_clangMetadata.end())
				{
					l_parent->totalChildrenCount++;
					if (kind == CXCursorKind::CXCursor_FieldDecl)
					{
						l_parent->validChildrenCount++;
					}
				}

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
		case CXCursor_EnumConstantDecl:
			fileWriter->os << "DeclType::EnumConstant";
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
		switch (typeKind)
		{
		case CXType_Invalid:
			fileWriter->os << "TypeKind::Invalid";
			break;
		case CXType_Void:
			fileWriter->os << "TypeKind::Void";
			break;
		case CXType_Bool:
			fileWriter->os << "TypeKind::Bool";
			break;
		case CXType_Char_U:
			fileWriter->os << "TypeKind::UChar";
			break;
		case CXType_UChar:
			fileWriter->os << "TypeKind::UChar";
			break;
		case CXType_Char16:
			fileWriter->os << "TypeKind::Char16";
			break;
		case CXType_Char32:
			fileWriter->os << "TypeKind::Char32";
			break;
		case CXType_UShort:
			fileWriter->os << "TypeKind::UShort";
			break;
		case CXType_UInt:
			fileWriter->os << "TypeKind::UInt";
			break;
		case CXType_ULong:
			fileWriter->os << "TypeKind::ULong";
			break;
		case CXType_ULongLong:
			fileWriter->os << "TypeKind::ULongLong";
			break;
		case CXType_Char_S:
			fileWriter->os << "TypeKind::SChar";
			break;
		case CXType_SChar:
			fileWriter->os << "TypeKind::SChar";
			break;
		case CXType_WChar:
			fileWriter->os << "TypeKind::WChar";
			break;
		case CXType_Short:
			fileWriter->os << "TypeKind::SShort";
			break;
		case CXType_Int:
			fileWriter->os << "TypeKind::SInt";
			break;
		case CXType_Long:
			fileWriter->os << "TypeKind::SLong";
			break;
		case CXType_LongLong:
			fileWriter->os << "TypeKind::SLongLong";
			break;
		case CXType_Float:
			fileWriter->os << "TypeKind::Float";
			break;
		case CXType_Double:
			fileWriter->os << "TypeKind::Double";
			break;
		case CXType_Pointer:
			fileWriter->os << "TypeKind::Pointer";
			break;
		case CXType_Enum:
			fileWriter->os << "TypeKind::Custom";
			break;
		case CXType_Typedef:
			fileWriter->os << "TypeKind::Custom";
			break;
		default:
			fileWriter->os << "TypeKind::Invalid";
			break;
		}
	}

	void writeMember(const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "\"" << clangMetadata.name << "\", ";

		writeCursorKind(clangMetadata.cursorKind, fileWriter);

		fileWriter->os << ", ";

		writeAccessSpecifier(clangMetadata.accessSpecifier, fileWriter);

		fileWriter->os << ", ";

		writeTypeKind(clangMetadata.typeKind, fileWriter);

		fileWriter->os << ", " << "\"" << clangMetadata.typeName << "\"";
	}

	void writeMetadataDefi(const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "Metadata ";
		fileWriter->os << "refl_" << clangMetadata.name << " = { ";

		writeMember(clangMetadata, fileWriter);

		fileWriter->os << " }";
	}

	void writeMetadataGetter(const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "template<>" << std::endl;
		fileWriter->os << "inline Metadata InnoMetadata::GetMetadata<" << clangMetadata.typeName << ">()" << std::endl;
		fileWriter->os << "{" << std::endl;
		fileWriter->os << "    return refl_" << clangMetadata.name << ";" << std::endl;
		fileWriter->os << "}" << std::endl;
		fileWriter->os << std::endl;
	}

	void writeSerializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "template<>" << std::endl;
		fileWriter->os << "inline void InnoSerializer::to_json<" << clangMetadata.typeName << ">(json& j, const " << clangMetadata.typeName << "& rhs)" << std::endl;
		fileWriter->os << "{" << std::endl;
		fileWriter->os << "  j = json" << std::endl;
		fileWriter->os << "  {";
		for (size_t j = 0; j < clangMetadata.totalChildrenCount; j++)
		{
			auto l_childClangMetaData = m_clangMetadata[index + j + 1];
			if (l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FieldDecl && l_childClangMetaData.accessSpecifier == CX_CXXAccessSpecifier::CX_CXXPublic)
			{
				fileWriter->os << std::endl;
				fileWriter->os << "   { \"" << l_childClangMetaData.name << "\", rhs." << l_childClangMetaData.name << " },";
			}
		}
		fileWriter->os << std::endl;
		fileWriter->os << "  };" << std::endl;
		fileWriter->os << "}" << std::endl;
		fileWriter->os << std::endl;
	}

	void writeDeserializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "template<>" << std::endl;
		fileWriter->os << "inline void InnoSerializer::from_json<" << clangMetadata.typeName << ">(const json& j, " << clangMetadata.typeName << "& rhs)" << std::endl;
		fileWriter->os << "{" << std::endl;
		for (size_t j = 0; j < clangMetadata.totalChildrenCount; j++)
		{
			auto l_childClangMetaData = m_clangMetadata[index + j + 1];
			if (l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FieldDecl && l_childClangMetaData.accessSpecifier == CX_CXXAccessSpecifier::CX_CXXPublic)
			{
				fileWriter->os << "  rhs." << l_childClangMetaData.name;
				//@TODO: Deal with pointer
				fileWriter->os << " = j[\"" << l_childClangMetaData.name << "\"];";

				fileWriter->os << std::endl;
			}
		}
		fileWriter->os << "}" << std::endl;
		fileWriter->os << std::endl;
	}

	void writeFile(FileWriter* fileWriter)
	{
		auto l_clangMetadataCount = m_clangMetadata.size();

		fileWriter->os << "#pragma once" << std::endl;
		fileWriter->os << "#include \"../../Source/Engine/Common/InnoMetadata.h\"" << std::endl;
		fileWriter->os << "using namespace InnoMetadata;" << std::endl;
		fileWriter->os << std::endl;

		for (size_t i = 0; i < l_clangMetadataCount; i++)
		{
			auto l_clangMetadata = m_clangMetadata[i];

			if (l_clangMetadata.cursorKind == CXCursorKind::CXCursor_CXXMethod || l_clangMetadata.cursorKind == CXCursorKind::CXCursor_ParmDecl)
			{
				//fileWriter->os << ", ";
				//fileWriter->os << l_clangMetadata.arraySize << ", ";
				//if (l_clangMetadata.cursorKind == CXCursorKind::CXCursor_CXXMethod || l_clangMetadata.cursorKind == CXCursorKind::CXCursor_ParmDecl)
				//{
				//	writeTypeKind(l_clangMetadata.returnTypeKind, fileWriter);
				//}
				//else
				//{
				//	fileWriter->os << "Invalid";
				//}
			}
			if (l_clangMetadata.cursorKind == CXCursorKind::CXCursor_StructDecl || l_clangMetadata.cursorKind == CXCursorKind::CXCursor_ClassDecl)
			{
				writeMetadataDefi(l_clangMetadata, fileWriter);
				fileWriter->os << ";" << std::endl;

				if (l_clangMetadata.validChildrenCount)
				{
					fileWriter->os << "Metadata ";
					fileWriter->os << "refl_" << l_clangMetadata.name << "_member" << "[" << l_clangMetadata.validChildrenCount << "]" << " = " << std::endl << "{";
					for (size_t j = 0; j < l_clangMetadata.totalChildrenCount; j++)
					{
						auto l_childClangMetaData = m_clangMetadata[i + j + 1];
						if (l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FieldDecl)
						{
							fileWriter->os << std::endl;
							fileWriter->os << " {";
							writeMember(l_childClangMetaData, fileWriter);
							fileWriter->os << " }, ";
						}
					}
					fileWriter->os << std::endl;
					fileWriter->os << "};" << std::endl;
				}

				//writeSerializerDefi(i, l_clangMetadata, fileWriter);
				//writeDeserializerDefi(i, l_clangMetadata, fileWriter);
				writeMetadataGetter(l_clangMetadata, fileWriter);
			}
		}
	}

	void parseContent(const std::string& fileName, FileWriter& fileWriter)
	{
		char* args[] = { "--language=c++" };

		auto index = clang_createIndex(0, 0);

		auto translationUnit = clang_parseTranslationUnit(index, fileName.c_str(), args, 1, nullptr, 0, CXTranslationUnit_SkipFunctionBodies);

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