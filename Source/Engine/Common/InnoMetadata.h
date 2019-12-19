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
		Dtor
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
		EnumConstant
	};

	struct Metadata
	{
		const char* name;
		DeclType declType;
		AccessType accessType;
		TypeKind typeKind;
		const char* typeName;
		bool isPtr;
		Metadata* base;
	};

	template<typename T>
	Metadata GetMetadata() {};
}

namespace InnoSerializer
{
	template<typename T>
	void to_json(json& j, const T& rhs) {};

	template<typename T>
	void from_json(const json& j, T& rhs) {};
}