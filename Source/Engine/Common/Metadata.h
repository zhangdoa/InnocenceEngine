#pragma once

namespace Inno
{
	namespace Metadata
	{
		enum class DeclType
		{
			Invalid,
			Namespace,
			Struct,
			Union,
			Class,
			Enum,
			EnumConstant,
			Var,
			Function,
			Parm,
			Ctor,
			Dtor,
			ClassTemplate,
			FunctionTemplate
		};

		enum class AccessType
		{
			Invalid,
			Public,
			Protected,
			Private
		};

		enum class TypeKind
		{
			Invalid,
			Custom,
			Void,
			Bool,
			UChar,
			SChar,
			WChar,
			Char16,
			Char32,
			UShort,
			UInt,
			ULong,
			ULongLong,
			SShort,
			SInt,
			SLong,
			SLongLong,
			Float,
			Double,

			Pointer,
			Enum,
			EnumConstant,
			TemplateParm
		};

		struct TypeInfo
		{
			const char* Name;
			DeclType DeclType;
			AccessType AccessType;
			TypeKind TypeKind;
			const char* TypeName;
			TypeInfo* TypeRef;
			bool IsPtr;
			TypeInfo* Base;
		};

		template<typename T>
		TypeInfo Get()
		{
			using orig_type = std::remove_const_t<std::remove_reference_t<decltype(T)>>;
			return Get<orig_type>();
		};
	}
}