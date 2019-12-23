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
		CXString displayName;
		CXString entityName;
		CXCursorKind cursorKind;
		CX_CXXAccessSpecifier accessSpecifier;
		CXTypeKind typeKind;
		CXString typeName;
		bool isPtr = false;
		bool isPOD = false;
		CXTypeKind returnTypeKind;
		CXString returnTypeName;
		size_t arraySize = 0;
		ClangMetadata* inheritanceBase = nullptr;
		ClangMetadata* semanticParent = nullptr;
		size_t totalChildrenCount = 0;
		size_t validChildrenCount = 0;
	};

	std::vector<CXString> m_includedFileName;
	std::vector<ClangMetadata> m_clangMetadata;

	void inclusionVisitor(CXFile included_file, CXSourceLocation* inclusion_stack, unsigned include_len, CXClientData client_data)
	{
		if (include_len > 0)
		{
			if (clang_Location_isFromMainFile(*inclusion_stack))
			{
				auto l_fileName = clang_getFileName(included_file);
				m_includedFileName.emplace_back(l_fileName);
			}
		}
	}

	CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData clientData)
	{
		CXSourceRange range = clang_getCursorExtent(cursor);
		CXSourceLocation location = clang_getRangeStart(range);

		if (clang_Location_isFromMainFile(location))
		{
			ClangMetadata l_metadata;

			auto kind = clang_getCursorKind(cursor);

			// We want a declaration, but not an access specifier, and we want the inheritance specifier, and only want the type reference of the base class, which coming after the CXCursor_CXXBaseSpecifier
			if ((clang_isDeclaration(kind)
				&& kind != CXCursorKind::CXCursor_CXXAccessSpecifier)
				|| kind == CXCursorKind::CXCursor_CXXBaseSpecifier
				|| (kind == CXCursorKind::CXCursor_TypeRef && m_clangMetadata[m_clangMetadata.size() - 1].cursorKind == CXCursorKind::CXCursor_CXXBaseSpecifier))
			{
				l_metadata.displayName = clang_getCursorDisplayName(cursor);
				l_metadata.entityName = clang_getCursorSpelling(cursor);

				l_metadata.cursorKind = kind;
				l_metadata.accessSpecifier = clang_getCXXAccessSpecifier(cursor);

				auto l_type = clang_getCursorType(cursor);

				if (l_type.kind == CXTypeKind::CXType_Pointer)
				{
					l_metadata.isPtr = true;
					l_type = clang_getPointeeType(l_type);
				}

				if (l_type.kind == CXTypeKind::CXType_ConstantArray)
				{
					l_metadata.arraySize = clang_getArraySize(l_type);
					l_type = clang_getArrayElementType(l_type);
				}

				l_metadata.isPOD = clang_isPODType(l_type);

				l_metadata.typeKind = l_type.kind;
				l_metadata.typeName = clang_getTypeSpelling(l_type);

				auto l_returnType = clang_getCursorResultType(cursor);

				l_metadata.returnTypeKind = l_returnType.kind;
				l_metadata.returnTypeName = clang_getTypeSpelling(l_returnType);

				auto l_semanticParent = parent;
				auto l_semanticParentName = clang_getCursorDisplayName(l_semanticParent);

				auto& l_parent = std::find_if(m_clangMetadata.begin(), m_clangMetadata.end(),
					[&](ClangMetadata& parent)
				{
					return !strcmp(clang_getCString(parent.displayName), clang_getCString(l_semanticParentName));
				});

				if (l_parent != m_clangMetadata.end())
				{
					l_parent->totalChildrenCount++;
					if (kind == CXCursorKind::CXCursor_FieldDecl || kind == CXCursorKind::CXCursor_CXXMethod || kind == CXCursorKind::CXCursor_EnumConstantDecl)
					{
						l_parent->validChildrenCount++;
						auto l_parentIndex = l_parent - m_clangMetadata.begin();
						l_metadata.semanticParent = &m_clangMetadata[l_parentIndex];
					}
				}
				m_clangMetadata.emplace_back(l_metadata);
			}
		}

		return CXChildVisit_Recurse;
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
		case CXType_Record:
			fileWriter->os << "TypeKind::Custom";
			break;
		case CXType_Enum:
			fileWriter->os << "TypeKind::Enum";
			break;
		case CXType_Typedef:
			fileWriter->os << "TypeKind::Custom";
			break;
		default:
			fileWriter->os << "TypeKind::Invalid";
			break;
		}
	}

	void assignBase()
	{
		auto l_clangMetadataCount = m_clangMetadata.size();

		for (size_t i = 0; i < l_clangMetadataCount; i++)
		{
			auto& l_clangMetadata = m_clangMetadata[i];

			if (l_clangMetadata.cursorKind == CXCursorKind::CXCursor_CXXBaseSpecifier)
			{
				m_clangMetadata[i - 1].inheritanceBase = &m_clangMetadata[i + 1];
			}
		}
	}

	void writeMetadataMember(const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		if (clangMetadata.cursorKind == CXCursorKind::CXCursor_CXXMethod)
		{
			std::string l_funcSign = clang_getCString(clangMetadata.displayName);
			auto l_parentName = clang_getCString(clangMetadata.semanticParent->displayName);

			l_funcSign = l_parentName + l_funcSign;

			std::hash<std::string> l_hasher;
			auto l_nameHash = l_hasher(l_funcSign);

			auto l_funcName = clang_getCString(clangMetadata.entityName);
			fileWriter->os << "\"" << l_parentName << "_" << l_funcName << "_" << l_nameHash << "\", ";
		}
		else
		{
			fileWriter->os << "\"" << clang_getCString(clangMetadata.displayName) << "\", ";
		}

		writeCursorKind(clangMetadata.cursorKind, fileWriter);

		fileWriter->os << ", ";

		writeAccessSpecifier(clangMetadata.accessSpecifier, fileWriter);

		fileWriter->os << ", ";

		writeTypeKind(clangMetadata.typeKind, fileWriter);

		fileWriter->os << ", " << "\"" << clang_getCString(clangMetadata.typeName) << "\"";

		if (clangMetadata.isPtr)
		{
			fileWriter->os << ", true";
		}
		else
		{
			fileWriter->os << ", false";
		}

		if (clangMetadata.inheritanceBase != nullptr)
		{
			fileWriter->os << ", &refl_" << clang_getCString(clangMetadata.inheritanceBase->typeName);
		}
		else
		{
			fileWriter->os << ", nullptr";
		}
	}

	void writeMetadataDefi(const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "Metadata refl_";

		fileWriter->os << clang_getCString(clangMetadata.displayName) << " = { ";

		writeMetadataMember(clangMetadata, fileWriter);

		fileWriter->os << " }";
	}

	void writeChildrenMetadataDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "Metadata refl_" << clang_getCString(clangMetadata.displayName) << "_member[" << clangMetadata.validChildrenCount << "] = \n{";

		auto l_startOffset = 1;
		if (clangMetadata.inheritanceBase != nullptr)
		{
			l_startOffset = 2;
		}

		for (size_t j = 0; j < clangMetadata.totalChildrenCount; j++)
		{
			auto l_childClangMetaData = m_clangMetadata[index + j + l_startOffset];
			if (l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FieldDecl
				|| l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_EnumConstantDecl
				|| l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_CXXMethod
				)
			{
				fileWriter->os << "\n\t{ ";
				writeMetadataMember(l_childClangMetaData, fileWriter);
				fileWriter->os << " }, ";
			}
		}
		fileWriter->os << "\n};\n";
	}

	void writeMetadataGetter(const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "template<>\ninline Metadata InnoMetadata::GetMetadata<" << clang_getCString(clangMetadata.typeName) << ">()\n";
		fileWriter->os << "{\n\treturn refl_" << clang_getCString(clangMetadata.displayName) << ";\n}\n\n";
	}

	void writeSerializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "template<>\ninline void InnoSerializer::to_json<" << clang_getCString(clangMetadata.typeName) << ">(json& j, const " << clang_getCString(clangMetadata.typeName) << "& rhs)\n{\n\tj = json\n\t{";

		auto l_startOffset = 1;
		if (clangMetadata.inheritanceBase != nullptr)
		{
			l_startOffset = 2;
		}

		for (size_t j = 0; j < clangMetadata.totalChildrenCount; j++)
		{
			auto l_childClangMetaData = m_clangMetadata[index + j + l_startOffset];

			if (l_childClangMetaData.arraySize > 0)
			{
				auto lss = l_childClangMetaData;
			}

			if (l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FieldDecl && l_childClangMetaData.accessSpecifier == CX_CXXAccessSpecifier::CX_CXXPublic)
			{
				auto l_name = clang_getCString(l_childClangMetaData.displayName);

				fileWriter->os << "\n\t\t{ \"" << l_name << "\", ";

				if (l_childClangMetaData.arraySize > 0)
				{
					fileWriter->os << "\n\t\t\t{ \n";
					for (size_t i = 0; i < l_childClangMetaData.arraySize; i++)
					{
						fileWriter->os << "\t\t\t\trhs." << l_name << "[" << i << "]";
						if (i + 1 != l_childClangMetaData.arraySize)
						{
							fileWriter->os << ",";
						}
						fileWriter->os << "\n";
					}
					fileWriter->os << "\t\t\t},\n\t\t},";
				}
				else
				{
					//@TODO: Deal with custom type pointer
					if (l_childClangMetaData.isPtr)
					{
						if (l_childClangMetaData.isPOD)
						{
							fileWriter->os << "*rhs." << l_name << " },";
						}
						else
						{
							fileWriter->os << "nullptr },";
						}
					}
					else
					{
						fileWriter->os << "rhs." << l_name << " },";
					}
				}
			}
		}
		fileWriter->os << "\n\t};\n}\n\n";
	}

	void writeDeserializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		fileWriter->os << "template<>\ninline void InnoSerializer::from_json<" << clang_getCString(clangMetadata.typeName) << ">(const json& j, " << clang_getCString(clangMetadata.typeName) << "& rhs)\n{\n";

		auto l_startOffset = 1;
		if (clangMetadata.inheritanceBase != nullptr)
		{
			l_startOffset = 2;
		}

		for (size_t j = 0; j < clangMetadata.totalChildrenCount; j++)
		{
			auto l_childClangMetaData = m_clangMetadata[index + j + l_startOffset];
			if (l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FieldDecl && l_childClangMetaData.accessSpecifier == CX_CXXAccessSpecifier::CX_CXXPublic)
			{
				auto l_name = clang_getCString(l_childClangMetaData.displayName);

				if (l_childClangMetaData.arraySize > 0)
				{
					for (size_t i = 0; i < l_childClangMetaData.arraySize; i++)
					{
						fileWriter->os << "\trhs." << l_name << "[" << i << "] = j[\"" << l_name << "\"][" << i << "];\n";
					}
				}
				else
				{
					//@TODO: Deal with custom type pointer
					if (l_childClangMetaData.isPtr)
					{
						if (l_childClangMetaData.isPOD)
						{
							fileWriter->os << "\t*rhs." << l_name << " = j[\"" << l_name << "\"];\n";
						}
					}
					else
					{
						fileWriter->os << "\trhs." << l_name << " = j[\"" << l_name << "\"];\n";
					}
				}
			}
		}
	}

	void writeIncludedHeaders(FileWriter* fileWriter)
	{
		auto l_includedFileNameCount = m_includedFileName.size();

		for (size_t i = 0; i < l_includedFileNameCount; i++)
		{
			auto l_name = clang_getCString(m_includedFileName[i]);
			fileWriter->os << "#include \"" << l_name << "\"\n";
		}
		fileWriter->os << "\n";
	}

	void writeSector(size_t index, const ClangMetadata& clangMetadata, FileWriter * fileWriter)
	{
		writeMetadataDefi(clangMetadata, fileWriter);
		fileWriter->os << ";\n";

		if (clangMetadata.validChildrenCount)
		{
			writeChildrenMetadataDefi(index, clangMetadata, fileWriter);
		}
	}

	void writeFile(FileWriter* fileWriter)
	{
		auto l_clangMetadataCount = m_clangMetadata.size();

		fileWriter->os << "#pragma once\n";
		fileWriter->os << "#include \"../../Source/Engine/Common/InnoMetadata.h\"\n";
		fileWriter->os << "using namespace InnoMetadata;\n";
		fileWriter->os << "\n";

		//writeIncludedHeaders(fileWriter);

		for (size_t i = 0; i < l_clangMetadataCount; i++)
		{
			auto& l_clangMetadata = m_clangMetadata[i];

			if (l_clangMetadata.cursorKind == CXCursorKind::CXCursor_EnumDecl)
			{
				writeSector(i, l_clangMetadata, fileWriter);
			}
			if (l_clangMetadata.cursorKind == CXCursorKind::CXCursor_StructDecl || l_clangMetadata.cursorKind == CXCursorKind::CXCursor_ClassDecl)
			{
				writeSector(i, l_clangMetadata, fileWriter);

				if (l_clangMetadata.validChildrenCount)
				{
					//writeSerializerDefi(i, l_clangMetadata, fileWriter);
					//writeDeserializerDefi(i, l_clangMetadata, fileWriter);
				}

				writeMetadataGetter(l_clangMetadata, fileWriter);
			}
		}

		fileWriter->os << std::endl;

		for (size_t i = 0; i < l_clangMetadataCount; i++)
		{
			clang_disposeString(m_clangMetadata[i].displayName);
			clang_disposeString(m_clangMetadata[i].entityName);
			clang_disposeString(m_clangMetadata[i].typeName);
			clang_disposeString(m_clangMetadata[i].returnTypeName);
		}
	}

	void parseContent(const std::string& fileName, FileWriter& fileWriter)
	{
		char* args[] = { "--language=c++" };

		auto index = clang_createIndex(0, 0);

		auto translationUnit = clang_parseTranslationUnit(index, fileName.c_str(), args, 1, nullptr, 0, CXTranslationUnit_SkipFunctionBodies);

		// @TODO: Reserve with a meaningful size
		m_includedFileName.reserve(128);
		m_clangMetadata.reserve(8192);

		auto cursor = clang_getTranslationUnitCursor(translationUnit);

		clang_getInclusions(translationUnit, inclusionVisitor, nullptr);

		m_includedFileName.shrink_to_fit();

		clang_visitChildren(cursor, visitor, nullptr);

		assignBase();

		writeFile(&fileWriter);

		clang_disposeTranslationUnit(translationUnit);

		clang_disposeIndex(index);
	}
}

using namespace InnoReflector;

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " Input" << argv[1] << " Output" << argv[2] << std::endl;
		return 0;
	}

	auto l_inputFilePath = fs::path(argv[1]);
	auto l_inputFileName = l_inputFilePath.generic_string();

	auto l_outputFilePath = fs::path(argv[2]);
	auto l_outputFileName = l_outputFilePath.generic_string();

	FileWriter l_fileWriter;

	l_fileWriter.os.open(l_outputFileName);

	parseContent(l_inputFileName, l_fileWriter);

	l_fileWriter.os.close();

	return 0;
}