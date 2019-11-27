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
	virtual bool Setup() override;
	virtual bool Initialize() override;
	virtual bool PrepareCommandList() override;
	virtual bool ExecuteCommandList() override;
	virtual bool Terminate() override;
};