#include "../../Engine/Common/InnoType.h"
#include "../../Engine/Core/IRenderingClient.h"

namespace InnoBaker
{
	void Setup();
	void BakeProbeCache(const char* sceneName);
	void BakeBrickCache(const char* surfelCacheFileName);
	void BakeBrick(const char* brickCacheFileName);
	void BakeBrickFactor(const char* brickFileName);
}

class InnoBakerRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	bool Setup() override;
	bool Initialize() override;
	bool PrepareCommandList() override;
	bool ExecuteCommandList() override;
	bool Terminate() override;
};