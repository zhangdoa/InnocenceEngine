#pragma once
#include "json/json.hpp"
using json = nlohmann::json;

namespace InnoMetadata
{
	enum class DeclType
	{
		Invalid = 0,
		Namespace = 1,
		Struct = 2,
		Union = 3,
		Class = 4,
		Enum = 5,
		EnumConstant = 6,
		Var = 7,
		Function = 8,
		Parm = 9,
		Ctor = 10,
		Dtor = 11
	};

	enum class AccessType
	{
		Invalid = 0,
		Public = 1,
		Protected = 2,
		Private = 3
	};

	enum class TypeKind
	{
		Invalid = 0,
		Custom = 1,
		Void = 2,
		Bool = 3,
		UChar = 4,
		SChar = 5,
		WChar = 6,
		Char16 = 7,
		Char32 = 8,
		UShort = 9,
		UInt = 10,
		ULong = 11,
		ULongLong = 12,
		SShort = 13,
		SInt = 14,
		SLong = 15,
		SLongLong = 16,
		Float = 17,
		Double = 18,
		Pointer = 19
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