#pragma once
#include "json/json.hpp"
using json = nlohmann::json;

namespace InnoMetadata
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

	struct Metadata
	{
		const char* name;
		DeclType declType;
		AccessType accessType;
		TypeKind typeKind;
		const char* typeName;
		Metadata* typeRef;
		bool isPtr;
		Metadata* base;
	};

	template<typename T>
	Metadata GetMetadata() {};

	template<typename T>
	Metadata GetMetadata(const T& rhs)
	{
		using orig_type = std::remove_const_t<std::remove_reference_t<decltype(rhs)>>;
		return GetMetadata<orig_type>();
	};
}

namespace InnoSerializer
{
	template<typename T>
	void to_json(json& j, const T& rhs) {};

	template<typename T>
	void from_json(const json& j, T& rhs) {};
}