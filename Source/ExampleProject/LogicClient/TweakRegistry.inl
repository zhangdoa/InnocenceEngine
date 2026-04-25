#pragma once

#include "../../Engine/ThirdParty/ImGui/imgui.h"
#include "../../Engine/Common/LogService.h"
#include "../../Engine/Common/IOService.h"

#include "../../Engine/Engine.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

using namespace Inno;

namespace Inno
{
	// Lightweight runtime-tweak registry. Each TweakVar wraps a typed pointer
	// to a live value plus the metadata needed to draw an ImGui slider and
	// JSON-round-trip it. The registry is owned by whoever calls
	// TweakRegistry::Register*; ownership of the underlying value stays with
	// that caller (typically a long-lived class member).
	//
	// JSON serialization is hand-rolled rather than going through JSONWrapper
	// because that wrapper privately includes nlohmann/json — pulling it into
	// LogicClient would force a CMake include-path change. The format we emit
	// matches what nlohmann::ordered_json with std::setw(4) would produce for
	// a flat object of primitives, so a future migration to JSONWrapper would
	// be byte-compatible.
	struct TweakVar
	{
		enum class Type
		{
			Float,
			Int,
			Bool,
		};

		std::string m_Key;
		Type m_Type;
		void* m_ValuePtr;
		float m_FloatMin = 0.0f;
		float m_FloatMax = 1.0f;
		int m_IntMin = 0;
		int m_IntMax = 100;
	};

	class TweakRegistry
	{
	public:
		// Register a float slider. Pointer must outlive the registry.
		static void RegisterFloat(const std::string& in_Key, float* in_ValuePtr, float in_Min, float in_Max)
		{
			if (!in_ValuePtr)
			{
				Log(Warning, "TweakRegistry::RegisterFloat rejected for key [", in_Key.c_str(), "]: null value pointer.");
				return;
			}
			TweakVar l_Var;
			l_Var.m_Key = in_Key;
			l_Var.m_Type = TweakVar::Type::Float;
			l_Var.m_ValuePtr = in_ValuePtr;
			l_Var.m_FloatMin = in_Min;
			l_Var.m_FloatMax = in_Max;
			GetVars().push_back(l_Var);
		}

		// Register an int slider.
		static void RegisterInt(const std::string& in_Key, int* in_ValuePtr, int in_Min, int in_Max)
		{
			if (!in_ValuePtr)
			{
				Log(Warning, "TweakRegistry::RegisterInt rejected for key [", in_Key.c_str(), "]: null value pointer.");
				return;
			}
			TweakVar l_Var;
			l_Var.m_Key = in_Key;
			l_Var.m_Type = TweakVar::Type::Int;
			l_Var.m_ValuePtr = in_ValuePtr;
			l_Var.m_IntMin = in_Min;
			l_Var.m_IntMax = in_Max;
			GetVars().push_back(l_Var);
		}

		// Register a bool checkbox.
		static void RegisterBool(const std::string& in_Key, bool* in_ValuePtr)
		{
			if (!in_ValuePtr)
			{
				Log(Warning, "TweakRegistry::RegisterBool rejected for key [", in_Key.c_str(), "]: null value pointer.");
				return;
			}
			TweakVar l_Var;
			l_Var.m_Key = in_Key;
			l_Var.m_Type = TweakVar::Type::Bool;
			l_Var.m_ValuePtr = in_ValuePtr;
			GetVars().push_back(l_Var);
		}

		// Draw all registered vars inside an existing ImGui window. Returns
		// true if any value changed this frame.
		static bool DrawAllImGui()
		{
			bool l_AnyChanged = false;
			for (auto& l_Var : GetVars())
			{
				switch (l_Var.m_Type)
				{
				case TweakVar::Type::Float:
				{
					float* l_Value = static_cast<float*>(l_Var.m_ValuePtr);
					if (ImGui::SliderFloat(l_Var.m_Key.c_str(), l_Value, l_Var.m_FloatMin, l_Var.m_FloatMax))
						l_AnyChanged = true;
					break;
				}
				case TweakVar::Type::Int:
				{
					int* l_Value = static_cast<int*>(l_Var.m_ValuePtr);
					if (ImGui::SliderInt(l_Var.m_Key.c_str(), l_Value, l_Var.m_IntMin, l_Var.m_IntMax))
						l_AnyChanged = true;
					break;
				}
				case TweakVar::Type::Bool:
				{
					bool* l_Value = static_cast<bool*>(l_Var.m_ValuePtr);
					if (ImGui::Checkbox(l_Var.m_Key.c_str(), l_Value))
						l_AnyChanged = true;
					break;
				}
				}
			}
			return l_AnyChanged;
		}

		// Emit a flat JSON object containing every registered key/value.
		// Format mirrors `json::dump(4)` output: one key per line, 4-space
		// indent, lexical key order matches registration order.
		static std::string SerializeToJSON()
		{
			std::ostringstream l_Out;
			l_Out << "{\n";
			const auto& l_Vars = GetVars();
			for (size_t i = 0; i < l_Vars.size(); ++i)
			{
				const auto& l_Var = l_Vars[i];
				l_Out << "    \"" << l_Var.m_Key << "\": ";
				switch (l_Var.m_Type)
				{
				case TweakVar::Type::Float:
					l_Out << *static_cast<float*>(l_Var.m_ValuePtr);
					break;
				case TweakVar::Type::Int:
					l_Out << *static_cast<int*>(l_Var.m_ValuePtr);
					break;
				case TweakVar::Type::Bool:
					l_Out << (*static_cast<bool*>(l_Var.m_ValuePtr) ? "true" : "false");
					break;
				}
				if (i + 1 < l_Vars.size())
					l_Out << ",";
				l_Out << "\n";
			}
			l_Out << "}\n";
			return l_Out.str();
		}

		// Parse a flat JSON object and apply matching keys to live values.
		// Tolerates extra whitespace and missing fields. On any parse error,
		// logs a warning and leaves remaining values untouched. This is a
		// deliberately tiny scanner — it only handles the format we emit
		// (primitives, no nesting, no arrays). Anything richer is a sign
		// the registry has outgrown this helper and should migrate to
		// JSONWrapper proper.
		static void ApplyFromJSON(const std::string& in_Body)
		{
			auto l_FindKey = [&](const std::string& in_Key, std::string& out_RawValue) -> bool
			{
				std::string l_Needle = "\"" + in_Key + "\"";
				size_t l_Pos = in_Body.find(l_Needle);
				if (l_Pos == std::string::npos)
					return false;
				l_Pos = in_Body.find(':', l_Pos);
				if (l_Pos == std::string::npos)
					return false;
				++l_Pos;
				while (l_Pos < in_Body.size() && std::isspace(static_cast<unsigned char>(in_Body[l_Pos])))
					++l_Pos;
				size_t l_Start = l_Pos;
				while (l_Pos < in_Body.size() && in_Body[l_Pos] != ',' && in_Body[l_Pos] != '}' && in_Body[l_Pos] != '\n')
					++l_Pos;
				out_RawValue = in_Body.substr(l_Start, l_Pos - l_Start);
				while (!out_RawValue.empty() && std::isspace(static_cast<unsigned char>(out_RawValue.back())))
					out_RawValue.pop_back();
				return !out_RawValue.empty();
			};

			for (auto& l_Var : GetVars())
			{
				std::string l_Raw;
				if (!l_FindKey(l_Var.m_Key, l_Raw))
					continue;
				try
				{
					switch (l_Var.m_Type)
					{
					case TweakVar::Type::Float:
						*static_cast<float*>(l_Var.m_ValuePtr) = std::stof(l_Raw);
						break;
					case TweakVar::Type::Int:
						*static_cast<int*>(l_Var.m_ValuePtr) = std::stoi(l_Raw);
						break;
					case TweakVar::Type::Bool:
						*static_cast<bool*>(l_Var.m_ValuePtr) = (l_Raw == "true" || l_Raw == "1");
						break;
					}
				}
				catch (const std::exception& l_Ex)
				{
					Log(Warning, "TweakRegistry: failed to parse key [", l_Var.m_Key.c_str(), "] value [", l_Raw.c_str(), "]: ", l_Ex.what());
				}
			}
		}

		// Load + apply registered keys from a JSON file at an asset-relative
		// path (resolved against IOService::getDataDirectory). Returns true
		// when the file existed and was applied. Logs the loaded values on
		// success so a runtime confirms boot-time tunables in normal-mode
		// logs without -loglevel 1.
		static bool LoadFromFile(const char* in_AssetRelPath)
		{
			auto* l_IO = g_Engine->Get<IOService>();
			std::string l_FullPath = l_IO->getDataDirectory() + in_AssetRelPath;
			std::ifstream l_File(l_FullPath);
			if (!l_File.is_open())
			{
				Log(Verbose, "TweakRegistry: file not found at ", l_FullPath.c_str(), " — keeping built-in defaults.");
				return false;
			}
			std::string l_Body((std::istreambuf_iterator<char>(l_File)), std::istreambuf_iterator<char>());
			ApplyFromJSON(l_Body);
			Log(Success, "TweakRegistry: loaded from ", l_FullPath.c_str(), ".");
			return true;
		}

		// Open an ImGui window with all registered tweaks plus a Save
		// button; auto-saves to in_AssetRelPath whenever any value changes.
		// Caller wraps this in a std::function and registers the function
		// pointer with ImGuiWrapper::AddUserDrawCallback.
		static void DrawWindowAutoSaving(const char* in_WindowTitle, const char* in_AssetRelPath)
		{
			ImGui::Begin(in_WindowTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			bool l_Changed = DrawAllImGui();
			if (ImGui::Button("Save"))
				l_Changed = true;
			if (l_Changed)
				SaveToFile(in_AssetRelPath);
			ImGui::End();
		}

		// Snapshot every registered live value to a JSON file at an asset-
		// relative path. Creates the parent directory on demand.
		static bool SaveToFile(const char* in_AssetRelPath)
		{
			auto* l_IO = g_Engine->Get<IOService>();
			std::string l_FullPath = l_IO->getDataDirectory() + in_AssetRelPath;
			std::error_code l_Ec;
			std::filesystem::create_directories(std::filesystem::path(l_FullPath).parent_path(), l_Ec);
			if (l_Ec)
			{
				Log(Warning, "TweakRegistry: failed to create config directory (", l_Ec.message().c_str(), "); save aborted.");
				return false;
			}
			std::ofstream l_Out(l_FullPath, std::ios::out | std::ios::trunc | std::ios::binary);
			if (!l_Out.is_open())
			{
				Log(Warning, "TweakRegistry: save failed for ", l_FullPath.c_str(), ".");
				return false;
			}
			l_Out << SerializeToJSON();
			l_Out.close();
			// DrawWindowAutoSaving calls SaveToFile on every slider tick; logging
			// every successful write floods the console during a single drag.
			// Emit Success only when the destination path changes (first-ever
			// save, or a switch to a different config slot) so the newsworthy
			// "we just wrote the defaults out" event still surfaces in normal
			// logs while steady-state autosaves stay quiet. Failures still log
			// unconditionally above.
			std::string& l_LastPath = GetLastSavedPath();
			if (l_LastPath != l_FullPath)
			{
				Log(Success, "TweakRegistry: saved to ", l_FullPath.c_str(), ".");
				l_LastPath = l_FullPath;
			}
			return true;
		}

		// Replace the registered set. Used when an owner tears down (e.g.
		// scene reload re-creates Player) so stale pointers don't leak into
		// the next session's UI.
		static void Clear()
		{
			GetVars().clear();
		}

	private:
		// Function-local-static storage. Keeps the helper header-only without
		// a separate translation unit.
		static std::vector<TweakVar>& GetVars()
		{
			static std::vector<TweakVar> s_Vars;
			return s_Vars;
		}

		static std::string& GetLastSavedPath()
		{
			static std::string s_LastSavedPath;
			return s_LastSavedPath;
		}
	};
}
