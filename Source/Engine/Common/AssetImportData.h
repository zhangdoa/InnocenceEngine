#pragma once
#include <functional>

namespace Inno
{
	/**
	 * Callback for asset import progress updates.
	 * float progress: 0.0 to 1.0 (or -1.0 for error)
	 * const char* name: name of the asset being processed
	 */
	typedef std::function<void(float, const char*)> AssetImportProgressCallback;
}
