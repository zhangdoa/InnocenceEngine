#pragma once

#include "../../Engine/Common/STL14.h"
#include "../../Engine/Common/STL17.h"
#include "../../Engine/Common/Metadata.h"
#include <clang-c/Index.h>

namespace Reflector
{
	struct FileWriter
	{
		std::ofstream os;
		std::string inputFileName;
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

	inline std::vector<CXSourceLocation> m_includedFileSourceLocation;
	inline std::vector<CXString> m_includedFileName;
	inline std::vector<ClangMetadata> m_clangMetadata;

	// --- Parse (Reflector_Parse.cpp) ---
	void inclusionVisitor(CXFile included_file, CXSourceLocation* inclusion_stack, unsigned include_len, CXClientData client_data);
	CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData clientData);
	void assignBase();
	void parseContent(const std::string& fileName, FileWriter& fileWriter);

	// --- Enum writers (Reflector_EnumWriters.cpp) ---
	void writeCursorKind(CXCursorKind typeKind, FileWriter* fileWriter);
	void writeAccessSpecifier(CX_CXXAccessSpecifier accessSpecifier, FileWriter* fileWriter);
	void writeTypeKind(CXTypeKind typeKind, FileWriter* fileWriter);

	// --- Metadata writers (Reflector_MetadataWriters.cpp) ---
	std::string flattenClangTypeName(const CXString& typeName);
	void writeMetadataMember(const ClangMetadata& clangMetadata, FileWriter* fileWriter);
	void writeMetadataDefi(const ClangMetadata& clangMetadata, FileWriter* fileWriter);
	void writeChildrenMetadataDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter);
	void writeMetadataGetter(const ClangMetadata& clangMetadata, FileWriter* fileWriter);
	void writeSerializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter);
	void writeDeserializerDefi(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter);
	void writeIncludedHeaders(FileWriter* fileWriter);
}
