#include "Engine_Internal.h"
#include "Common/LogService.h"
#include "Services/AssetService.h"
#include <chrono>
#include <thread>

using namespace Inno;

bool Engine::Run()
{
	// Bake mode (TASK-68): headless, one-shot import-then-exit. The normal
	// Run loop depends on WindowSystem being Activated; in bake mode we use
	// the HeadlessWindowService and never enter frame pacing.
	//
	// Per-file parallelism (TASK-70 axis 1): each path runs on its own
	// std::thread. Inside ImportSync, ProcessAssimpScene fans out mesh and
	// material work to TaskScheduler workers (axis 2). If the outer file loop
	// also ran on scheduler workers, a worker's outer task would call
	// Thread::AddTask on itself — Thread::AddTask requires the target thread
	// to leave Busy, which can't happen while the outer task blocks on its
	// own sub-tasks. std::thread keeps the orchestrator off the worker pool
	// so sub-task submissions make forward progress. AssimpWrapper::Import is
	// thread-safe: each call constructs its own local Assimp::Importer, writes
	// to unique filenames, and goes through the per-type shared_mutex-guarded
	// AssetService registries.
	if (m_pImpl->m_initConfig.isBakeMode)
	{
		const std::string l_list(m_pImpl->m_initConfig.bakeInputs);
		auto* l_assetService = Get<AssetService>();

		struct BakeTask
		{
			std::string path;
			bool ok{false};
			int64_t elapsedMs{0};
		};
		std::vector<BakeTask> l_tasks;

		size_t l_pos = 0;
		while (l_pos < l_list.size())
		{
			size_t l_sep = l_list.find(';', l_pos);
			if (l_sep == std::string::npos) l_sep = l_list.size();
			std::string l_path = l_list.substr(l_pos, l_sep - l_pos);
			l_pos = l_sep + 1;
			if (l_path.empty()) continue;
			l_tasks.push_back({ std::move(l_path), false, 0 });
		}

		const auto l_batchStart = std::chrono::steady_clock::now();

		std::vector<std::thread> l_threads;
		l_threads.reserve(l_tasks.size());
		for (auto& l_entry : l_tasks)
		{
			l_threads.emplace_back([l_assetService, &l_entry]()
			{
				const auto l_fileStart = std::chrono::steady_clock::now();
				const bool l_result = l_assetService->ImportSync(l_entry.path.c_str());
				l_entry.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - l_fileStart).count();
				l_entry.ok = l_result;
			});
		}
		for (auto& l_thread : l_threads)
			l_thread.join();

		uint32_t l_ok = 0, l_fail = 0;
		for (const auto& l_entry : l_tasks)
		{
			if (l_entry.ok)
			{
				Log(Success, "Bake: ", l_entry.path.c_str(), " imported in ", static_cast<uint64_t>(l_entry.elapsedMs), " ms.");
				++l_ok;
			}
			else
			{
				Log(Error, "Bake: ", l_entry.path.c_str(), " FAILED after ", static_cast<uint64_t>(l_entry.elapsedMs), " ms.");
				++l_fail;
			}
		}
		const auto l_totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - l_batchStart).count();
		Log(Success, "Bake complete: ", l_ok, " ok, ", l_fail, " failed, wall-clock ",
			static_cast<uint64_t>(l_totalMs), " ms across ", static_cast<uint32_t>(l_tasks.size()), " files.");
		return l_fail == 0;
	}

	while (1)
	{
		if (!ExecuteDefaultTask())
		{
			return false;
		}
	}
	return true;
}
