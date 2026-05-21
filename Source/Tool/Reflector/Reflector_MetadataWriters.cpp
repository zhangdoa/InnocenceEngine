#include "Reflector_Internal.h"

namespace Reflector
{
	std::string flattenClangTypeName(const CXString& typeName)
	{
		auto l_typeName = std::string(clang_getCString(typeName));
		std::replace(l_typeName.begin(), l_typeName.end(), ':', '_');
		l_typeName = "Metadata_" + l_typeName;

		return l_typeName;
	}

	void writeMetadataMember(const ClangMetadata& clangMetadata, FileWriter* fileWriter)
	{
		if (clangMetadata.cursorKind == CXCursorKind::CXCursor_CXXMethod
			|| clangMetadata.cursorKind == CXCursorKind::CXCursor_FunctionTemplate
			)
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
			fileWriter->os << "\"" << clang_getCString(clangMetadata.entityName) << "\", ";
		}

		writeCursorKind(clangMetadata.cursorKind, fileWriter);

		fileWriter->os << ", ";

		writeAccessSpecifier(clangMetadata.accessSpecifier, fileWriter);

		fileWriter->os << ", ";

		writeTypeKind(clangMetadata.typeKind, fileWriter);

		fileWriter->os << ", " << "\"" << clang_getCString(clangMetadata.typeName) << "\"";

		if (clangMetadata.cursorKind == CXCursorKind::CXCursor_FieldDecl
			&& (clangMetadata.typeKind == CXTypeKind::CXType_Record
				|| clangMetadata.typeKind == CXTypeKind::CXType_Enum))
		{
			fileWriter->os << ", &" << flattenClangTypeName(clangMetadata.typeName);
		}
		else
		{
			fileWriter->os << ", nullptr";
		}

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
			fileWriter->os << ", &" << flattenClangTypeName(clangMetadata.inheritanceBase->typeName);
		}
		else
		{
			fileWriter->os << ", nullptr";
		}
	}

	void writeMetadataDefi(const ClangMetadata& clangMetadata, FileWriter* fileWriter)
	{
		fileWriter->os << "inline Metadata::TypeInfo ";

		fileWriter->os << flattenClangTypeName(clangMetadata.typeName) << " = { ";

		writeMetadataMember(clangMetadata, fileWriter);

		fileWriter->os << " }";
	}

	void writeChildrenMetadataDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter)
	{
		fileWriter->os << "inline Metadata::TypeInfo " << flattenClangTypeName(clangMetadata.typeName) << "_Member[" << clangMetadata.validChildrenCount << "] = \n{";

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
				|| l_childClangMetaData.cursorKind == CXCursorKind::CXCursor_FunctionTemplate
				)
			{
				fileWriter->os << "\n\t{ ";
				writeMetadataMember(l_childClangMetaData, fileWriter);
				fileWriter->os << " }, ";
			}
		}
		fileWriter->os << "\n};\n";
	}

	void writeMetadataGetter(const ClangMetadata& clangMetadata, FileWriter* fileWriter)
	{
		fileWriter->os << "template<>\ninline Inno::Metadata::TypeInfo Inno::Metadata::Get<" << clang_getCString(clangMetadata.typeName) << ">()\n";
		fileWriter->os << "{\n\treturn " << flattenClangTypeName(clangMetadata.typeName) << ";\n}\n\n";
	}

	void writeSerializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter)
	{
		fileWriter->os << "template<>\ninline void Serializer::to_json<" << clang_getCString(clangMetadata.typeName) << ">(json& j, const " << clang_getCString(clangMetadata.typeName) << "& rhs)\n{\n\tj = json\n\t{";

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
				auto l_name = clang_getCString(l_childClangMetaData.entityName);

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

	void writeDeserializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter)
	{
		fileWriter->os << "template<>\ninline void Serializer::from_json<" << clang_getCString(clangMetadata.typeName) << ">(const json& j, " << clang_getCString(clangMetadata.typeName) << "& rhs)\n{\n";

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
				auto l_name = clang_getCString(l_childClangMetaData.entityName);

				if (l_childClangMetaData.arraySize > 0)
				{
					for (size_t i = 0; i < l_childClangMetaData.arraySize; i++)
					{
						fileWriter->os << "\trhs." << l_name << "[" << i << "] = j[\"" << l_name << "\"][" << i << "];\n";
					}
				}
				else
				{
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
		fileWriter->os << "}\n";
	}

	void writeIncludedHeaders(FileWriter* fileWriter)
	{
		auto l_includedFileNameCount = m_includedFileSourceLocation.size();

		for (size_t i = 0; i < l_includedFileNameCount; i++)
		{
			std::string l_name = clang_getCString(m_includedFileName[i]);
			size_t start_pos = l_name.find(".h");
			l_name.replace(start_pos, 2, ".h.refl");

			fileWriter->os << "#include " << l_name << "\n";
		}
		fileWriter->os << "\n";
	}
}
