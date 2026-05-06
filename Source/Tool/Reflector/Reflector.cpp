#include "Reflector_Internal.h"

#ifdef __GNUC__
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace Reflector
{
	void writeSector(size_t index, const ClangMetadata& clangMetadata, FileWriter* fileWriter)
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
		fileWriter->os << "#include \"" << fileWriter->inputFileName << "\"\n";
		fileWriter->os << "\n";
		fileWriter->os << "using namespace Inno;\n";
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
		m_includedFileSourceLocation.reserve(128);
		m_includedFileName.reserve(128);
		m_clangMetadata.reserve(8192);

		auto cursor = clang_getTranslationUnitCursor(translationUnit);

		clang_getInclusions(translationUnit, inclusionVisitor, nullptr);

		auto l_includedFileSourceLocationSize = m_includedFileSourceLocation.size();
		for (size_t i = 0; i < l_includedFileSourceLocationSize; i++)
		{
			auto l_token = clang_getToken(translationUnit, m_includedFileSourceLocation[i]);
			m_includedFileName.emplace_back(clang_getTokenSpelling(translationUnit, *l_token));
		}

		clang_visitChildren(cursor, visitor, nullptr);

		assignBase();

		writeFile(&fileWriter);

		clang_disposeTranslationUnit(translationUnit);

		clang_disposeIndex(index);
	}
}

using namespace Reflector;

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " Input" << argv[1] << std::endl;
		return 0;
	}

	auto l_inputFilePath = fs::path(argv[1]);
	auto l_inputFilePathStr = l_inputFilePath.generic_string();
	auto l_outputFilePathStr = l_inputFilePathStr + ".refl";

	FileWriter l_fileWriter;

	l_fileWriter.inputFileName = l_inputFilePath.filename().generic_string();
	l_fileWriter.os.open(l_outputFilePathStr);

	parseContent(l_inputFilePathStr, l_fileWriter);

	l_fileWriter.os.close();

	return 0;
}
