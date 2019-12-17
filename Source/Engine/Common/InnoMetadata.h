#pragma once
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
		Var = 6,
		Function = 7,
		Parm = 8,
		Ctor = 9,
		Dtor = 10
	};

	enum class AccessType
	{
		Invalid = 0,
		Public = 1,
		Protect = 2,
		Private = 3
	};

	struct Metadata
	{
		const char* name;
		DeclType declType;
		AccessType accessType;
	};

	template<typename T>
	Metadata GetMetadata(const T& rhs) {};
}
