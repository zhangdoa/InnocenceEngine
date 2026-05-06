#include "Reflector_Internal.h"

namespace Reflector
{
	void writeCursorKind(CXCursorKind typeKind, FileWriter* fileWriter)
	{
		fileWriter->os << "Metadata::DeclType::";
		switch (typeKind)
		{
		case CXCursor_UnexposedDecl:
			fileWriter->os << "Unexposed";
			break;
		case CXCursor_StructDecl:
			fileWriter->os << "Struct";
			break;
		case CXCursor_UnionDecl:
			fileWriter->os << "Union";
			break;
		case CXCursor_ClassDecl:
			fileWriter->os << "Class";
			break;
		case CXCursor_EnumDecl:
			fileWriter->os << "Enum";
			break;
		case CXCursor_FieldDecl:
			fileWriter->os << "Var";
			break;
		case CXCursor_EnumConstantDecl:
			fileWriter->os << "EnumConstant";
			break;
		case CXCursor_ParmDecl:
			fileWriter->os << "Parm";
			break;
		case CXCursor_CXXMethod:
			fileWriter->os << "Function";
			break;
		case CXCursor_Namespace:
			fileWriter->os << "Namespace";
			break;
		case CXCursor_Constructor:
			fileWriter->os << "Ctor";
			break;
		case CXCursor_Destructor:
			fileWriter->os << "Dtor";
			break;
		case CXCursor_FunctionTemplate:
			fileWriter->os << "FunctionTemplate";
			break;
		case CXCursor_ClassTemplate:
			fileWriter->os << "ClassTemplate";
			break;
		case CXCursor_DLLExport:
			break;
		case CXCursor_DLLImport:
			break;
		default:
			fileWriter->os << "Invalid";
			break;
		}
	}

	void writeAccessSpecifier(CX_CXXAccessSpecifier accessSpecifier, FileWriter* fileWriter)
	{
		fileWriter->os << "Metadata::AccessType::";

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
		fileWriter->os << "Metadata::TypeKind::";

		switch (typeKind)
		{
		case CXType_Invalid:
			fileWriter->os << "Invalid";
			break;
		case CXType_Void:
			fileWriter->os << "Void";
			break;
		case CXType_Bool:
			fileWriter->os << "Bool";
			break;
		case CXType_Char_U:
			fileWriter->os << "UChar";
			break;
		case CXType_UChar:
			fileWriter->os << "UChar";
			break;
		case CXType_Char16:
			fileWriter->os << "Char16";
			break;
		case CXType_Char32:
			fileWriter->os << "Char32";
			break;
		case CXType_UShort:
			fileWriter->os << "UShort";
			break;
		case CXType_UInt:
			fileWriter->os << "UInt";
			break;
		case CXType_ULong:
			fileWriter->os << "ULong";
			break;
		case CXType_ULongLong:
			fileWriter->os << "ULongLong";
			break;
		case CXType_Char_S:
			fileWriter->os << "SChar";
			break;
		case CXType_SChar:
			fileWriter->os << "SChar";
			break;
		case CXType_WChar:
			fileWriter->os << "WChar";
			break;
		case CXType_Short:
			fileWriter->os << "SShort";
			break;
		case CXType_Int:
			fileWriter->os << "SInt";
			break;
		case CXType_Long:
			fileWriter->os << "SLong";
			break;
		case CXType_LongLong:
			fileWriter->os << "SLongLong";
			break;
		case CXType_Float:
			fileWriter->os << "Float";
			break;
		case CXType_Double:
			fileWriter->os << "Double";
			break;
		case CXType_Pointer:
			fileWriter->os << "Pointer";
			break;
		case CXType_Record:
			fileWriter->os << "Custom";
			break;
		case CXType_Enum:
			fileWriter->os << "Enum";
			break;
		case CXType_Typedef:
			fileWriter->os << "Custom";
			break;
		default:
			fileWriter->os << "Invalid";
			break;
		}
	}
}
