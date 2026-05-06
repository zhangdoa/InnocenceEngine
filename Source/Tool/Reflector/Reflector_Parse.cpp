#include "Reflector_Internal.h"

namespace Reflector
{
	void inclusionVisitor(CXFile included_file, CXSourceLocation* inclusion_stack, unsigned include_len, CXClientData client_data)
	{
		if (include_len > 0)
		{
			if (clang_Location_isFromMainFile(*inclusion_stack))
			{
				m_includedFileSourceLocation.emplace_back(*inclusion_stack);
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

				if (kind == CXCursorKind::CXCursor_FieldDecl
					&& (l_type.kind == CXTypeKind::CXType_Record
						|| l_type.kind == CXTypeKind::CXType_Enum))
				{
					auto l_declCursor = clang_getTypeDeclaration(l_type);
					l_metadata.typeName = clang_getCursorSpelling(l_declCursor);
				}
				else
				{
					l_metadata.typeName = clang_getTypeSpelling(l_type);
				}

				auto l_returnType = clang_getCursorResultType(cursor);

				l_metadata.returnTypeKind = l_returnType.kind;
				l_metadata.returnTypeName = clang_getTypeSpelling(l_returnType);

				auto l_semanticParentName = clang_getCursorDisplayName(parent);

				auto& l_parent = std::find_if(m_clangMetadata.begin(), m_clangMetadata.end(),
					[&](ClangMetadata& parent)
				{
					return !strcmp(clang_getCString(parent.displayName), clang_getCString(l_semanticParentName));
				});

				if (l_parent != m_clangMetadata.end())
				{
					l_parent->totalChildrenCount++;
					if (kind == CXCursorKind::CXCursor_FieldDecl
						|| kind == CXCursorKind::CXCursor_CXXMethod
						|| kind == CXCursorKind::CXCursor_EnumConstantDecl
						|| kind == CXCursorKind::CXCursor_FunctionTemplate
						)
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
}
